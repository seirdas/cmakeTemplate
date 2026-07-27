#include "net/NetMgr.hpp"
#include "system/SystemMgr.hpp"

#if defined ASIO_NETWORK || defined ASIO_NETWORK_VERSION

    #include <asio.hpp>             // asio external lib
    #include "net/UdpSocket.hpp"    // Para conocer UdpSocket
    #include "net/netTypes.hpp"     // Para conocer NetPacket
    #include "files/JsonMgr.hpp"    // Para conocer json


    using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;
    using UDPSocketsVector = std::vector<std::shared_ptr<UdpSocket>>;

    // Implementación de miembros de la clase de asio (pimpl_)
    struct NetMgr::Impl {
        asio::io_context    io_context_;       // UN ÚNICO contexto de operaciones asíncronas para todo
        WorkGuard           work_guard_;       // RAII para mantener vivo el io
        UDPSocketsVector    udp_sockets_;      // Lista de receptores UDP registrados

        // Se declara aquí porque NetPacket está definido en UdpSocket, que sólo se incluye en este cpp
        std::deque<NetPacket>       udp_rcv_data_;      ///< Cola de datos recibidos por los sockets UDP

        // Este struct necesita inicializar io_context con work_guar, por eso declaramos el constructor
        Impl() : work_guard_(asio::make_work_guard(io_context_)) {}
    };

    // Función helper fuera de la clase para evitar esta transformación siempre que se use el strand
    asio::strand<asio::io_context::executor_type>& getAsioStrand(std::shared_ptr<UdpSocket> const& sock) {
        return *static_cast<asio::strand<asio::io_context::executor_type>*>(sock->getStrandNative());
    }


    // General ------------------------------------------------------------------------------

    NetMgr::NetMgr(std::size_t const& thread_count) :
        pimpl_(std::make_unique<Impl>()),
        initialized_(false),
        io_running_(false),
        sockets_running_(false),
        num_threads_(thread_count == 0 ? 1 : thread_count)
    {

    }

    NetMgr::~NetMgr() {
        close();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool NetMgr::init(void* config) {
        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        // No hago else para que no se queje si vuelve de OFFLINE->ONLINE

        // Evitar lanzar hilos si ya están corriendo
        if (sockets_running_) {
            SYS_WARN("NetMgr","Commanded start() when is already running");
            return true;
        }

        // Inicializar el io_context con varios hilos de recepción
        if (!io_running_) {
            pimpl_->io_context_.restart();
            SYS_INFO("NetMgr","Starting I/O context with " + std::to_string(num_threads_) + " threads...");
            for (std::size_t i = 0; i < num_threads_; ++i) {
                threads_.emplace_back([this]() {
                    pimpl_->io_context_.run();
                });
            }
            SYS_INFO("NetMgr","Network (io_context) running...");
            io_running_=true;
        }

        // Iniciar (de nuevo si aplica) los sockets
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Iniciar sockets
            for (auto& sock : pimpl_->udp_sockets_)
                asio::post(getAsioStrand(sock), [sock]{ 
                    sock->start(); 
                });
            
        }

        SYS_INFO("NetMgr","Sockets running");
        sockets_running_ = true;
        initialized_ = true;
        return true;
    }

    bool NetMgr::isInitialized() const {
        return initialized_;
    }

    void NetMgr::loadConfig(void* config) {

        if (!config) 
            return;
            
        // Se considera que la configuración se pasa como json    
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Thread_count
        jsonMgr.get_or_set(cfg, "num_threads", num_threads_);
        
        // Establecer dentro de rango: 0=auto, >max = max
        unsigned short max_threads = std::thread::hardware_concurrency();
        if (max_threads < 1) max_threads = 1;
        if (num_threads_ == 0) {
            SYS_INFO("NetMgr","Thread_count set auto to max hw concurrency (" + std::to_string(max_threads)+")");
            num_threads_ = max_threads;
        }
        if (num_threads_ > max_threads) {
            SYS_WARN("NetMgr","Warn: Thread_count higher than max allowed. thread_count set to max hw concurrency (" + std::to_string(max_threads)+")");
            num_threads_ = max_threads;
        }


        // hago un vector que apunte al array de los nodos json dentro del nodo principal
        std::vector<json*> config_net = jsonMgr.getArrayElements(cfg, "udpSockets");

        // Bucle que recorre los elementos de dentro del nodo
        short port = 0;
        std::string name = "";
        for (json* const cfg_node : config_net) {
            name = "";      // (re)inicializa el nombre
            port = 0;       // (re)inicializa el puerto

            // usamos get_or_set para leerlo del json y si no está rellenar el json
            jsonMgr.get_or_set(cfg_node, "name", name); // busca el nombre
            jsonMgr.get_or_set(cfg_node, "port", port); // busca el puerto

            // si el nombre y el puerto no estan vacios, crea el socket
            if(!name.empty() && port > 0) {
                SYS_INFO("NetMgr","Registering socket from config...");
                addUdpSocket(name, port);
            }
        }

    }

    bool NetMgr::start() {
        return init();
    }

    void NetMgr::stop() {

        // Si los sockets ya están inactivos, no hacer nada
        if (!sockets_running_) return;

        // Parar la recepción de los sockets
        SYS_INFO("NetMgr","Stopping sockets...");
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Parar sockets (debería funcionar sin usar strand, parada directa)
            for (auto& sock : pimpl_->udp_sockets_) 
            asio::post(getAsioStrand(sock), [sock]{ 
                    sock->stop(); 
                });
        }

        sockets_running_ = false;
        udp_rcv_data_cv_.notify_all();   // desbloquea getNextPacket()
        SYS_INFO("NetMgr","All sockets stopped");
    }

    void NetMgr::close() {
        // Parar los sockets
        stop();

        // Soltar el work_guard para que io_context pueda drenar y terminar.
        pimpl_->work_guard_.reset();

        // Esperar a que los hilos salgan naturalmente (sin stop() forzado).
        //    io_context terminará solo cuando no queden handlers pendientes.
        for (auto& t : threads_)
            if (t.joinable()) t.join(); // Esperar a los hilos de io_context
        io_running_ = false;
        threads_.clear();
    }

    bool NetMgr::isRunning() const {
        return sockets_running_;
    }


    // Gestión de sockets -------------------------------------------------------------------

    bool NetMgr::addUdpSocket(
        std::string         name,
        unsigned short      local_port, 
        const std::string&  local_ip, 
        unsigned int        rcv_packet_size
    ) 
    {
        // Rechazar datos inválidos
        if (local_port == 0) {
            SYS_WARN("NetMgr","Cannot add new socket: Invalid port: " + std::to_string(local_port));
            return false;
        }
        if (name.empty()) {
            SYS_WARN("NetMgr","Cannot add new socket: No name provided.");
            return false;
        }

        // Evitar duplicados por nombre y puerto
        {
            // Bloquear el acceso a la cola de otras llamadas
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Buscar duplicados
            for (const auto& sock : pimpl_->udp_sockets_) {
                if (sock->name() == name) {
                    SYS_WARN("NetMgr","Socket already exists with name " + name);;
                    return false;
                }
                if (sock->port() == local_port) {
                    SYS_WARN("NetMgr","Socket already exists with port " + std::to_string(local_port));
                    return false;
                }
            }
        }

        // Crear socket (aún no registrado)
        std::shared_ptr<UdpSocket> socket = std::make_shared<UdpSocket>(name, &pimpl_->io_context_);

        // [INYECCIÓN DEL CALLBACK]
        socket->setReceiveCallback([this](NetPacket pkt) {

            // Proteger la cola de datos del socket
            std::lock_guard<std::mutex> lock(udp_rcv_data_mtx_);

            // Agregar dato recibido a la cola general de NetMgr (si cabe)
            if (pimpl_->udp_rcv_data_.size() < MAX_QUEUE_ELEMENTS_)
                pimpl_->udp_rcv_data_.push_back(std::move(pkt));
            else
                SYS_WARN("NetMgr", "Central queue full, dropping packet from " + pkt.socket_name + ":" + std::to_string(pkt.port));

            // Notificar 
            udp_rcv_data_cv_.notify_all();
        });

        // Intentar inicializar
        SYS_INFO("NetMgr","Opening new socket...");
        if (!socket->init(local_port, local_ip, rcv_packet_size))
        {
            // No hay nada que limpiar, el puntero make_unique se destruye al salir.
            SYS_WARN("NetMgr","Failed to initialize socket on port " + std::to_string(local_port));
            return false;
        }

        {
            // Proteger el vector de sockets udp
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Insertar en el vector
            pimpl_->udp_sockets_.push_back(socket);
        }
        
        // "encender" el socket si estaban los demás corriendo
        if (sockets_running_)
            asio::post(getAsioStrand(socket), [socket]{ 
                socket->start(); 
            });

        SYS_INFO("NetMgr","Socket added on port " + std::to_string(local_port) );
        return true;
    }

    bool NetMgr::removeUdpSocket(std::string const& name) {
        // Protege el vector de sockets de otras llamadas
        std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

        // Llama función privada con lock adquirido
        return RemoveUdpSocketLocked(getSocketIndex(name));
    }

    bool NetMgr::removeUdpSocket(unsigned int port) {
        // Protege el vector de sockets de otras llamadas
        std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

        // Llama función privada con lock adquirido
        return RemoveUdpSocketLocked(getSocketIndex(port));
    }

    void NetMgr::printUdpSockets() const {

        SYS_INFO("NetMgr", "--- SOCKETS ENABLED ---");
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Recorrer e imprimir sockets
            for (unsigned int i = 0; i < pimpl_->udp_sockets_.size(); i++)
            SYS_INFO("NetMgr", 
                "[" + std::to_string(i) + "] Socket '" + 
                pimpl_->udp_sockets_[i]->name() + ":" + 
                std::to_string(pimpl_->udp_sockets_[i]->port() ) + "'"
            );
        }
        
    }

    bool NetMgr::socketExists(std::string const& socketname) const {
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Si hay un socket con ese nombre, devuelve true
            for (const auto& sock : pimpl_->udp_sockets_) 
                if (sock->name() == socketname) {
                    SYS_INFO("NetMgr","Socket exists function: '" + socketname + ":" + std::to_string(sock->port()) + "' found.");
                    return true;
                }
        }

        // Llegará aquí si no encuentra socket
        SYS_INFO("NetMgr","Socket exists function: '" + socketname + "' not found.");
        return false;
    }

    bool NetMgr::socketExists(unsigned short port) const {
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Si hay un socket con ese puerto, devuelve true
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->port() == port) {
                    SYS_INFO("NetMgr","Socket " + sock->name() + ":" + std::to_string(port) + "' found.");
                    return true;
                }
        }
        
        // Llegará aquí si no encuentra socket
        SYS_WARN("NetMgr","Socket with port " + std::to_string(port) + "' not found.");
        return false;
    }
    
    unsigned long long NetMgr::getLastPacketMs(std::string const& name) const {
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Si hay un socket con ese puerto, devuelve su tiempo
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->name() == name) 
                    return sock->getLastPacketMs();
        }
        
        // Llegará aquí si no encuentra socket
        SYS_WARN("NetMgr","Socket '" + name + "' not found.");
        return 0;
    }

    unsigned long long NetMgr::getLastPacketMs(unsigned short port) const {
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Si hay un socket con ese puerto, devuelve su tiempo
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->port() == port) 
                    return sock->getLastPacketMs();
        }
        
        // Llegará aquí si no encuentra socket
        SYS_WARN("NetMgr","Socket with port " + std::to_string(port) + "' not found.");
        return 0;
    }


    // Envío --------------------------------------------------------------------------------

    bool NetMgr::sendData(
        std::string socketname, 
        const std::vector<char>& data,
        const std::string& dest_ip,
        unsigned short dest_port
    ) 
    {
        // Si hay un socket con ese nombre, manda los datos
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Mandar paquete por nombre de socket
            for (const auto& sock : pimpl_->udp_sockets_) 
                if (sock->name() == socketname)
                    return sock->sendPacket(data, dest_ip, dest_port);
        }

        // Llegará aquí si no encuentra socket
        SYS_WARN("NetMgr","Socket '"+ socketname +"' not exists");
        return false;
    }

    bool NetMgr::sendData(
        unsigned short           local_port, 
        const std::vector<char>& data,
        const std::string&       dest_ip,
        unsigned short           dest_port
    ) 
    {
        // Si hay un socket con ese nombre, manda los datos
        {
            // Proteger acceso a vector de sockets
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

            // Mandar paquete por puerto de socket
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->port() == local_port) 
                    return sock->sendPacket(data, dest_ip, dest_port);
        }

        // Llegará aquí si no encuentra socket
        SYS_WARN("NetMgr","Socket not exists with port " + std::to_string(local_port));
        return false;
    }


    // Recepción ----------------------------------------------------------------------------

    std::vector<char> NetMgr::getNextUdpPacket(std::string* name, unsigned short* port) {
        std::unique_lock<std::mutex> lock(udp_rcv_data_mtx_);
        udp_rcv_data_cv_.wait(
            lock, [this] {
                return !pimpl_->udp_rcv_data_.empty() || !sockets_running_;
            }
        );

        if (pimpl_->udp_rcv_data_.empty())
            return {};   // red detenida, TWorker debe salir

        NetPacket pkt = std::move(pimpl_->udp_rcv_data_.front());
        pimpl_->udp_rcv_data_.pop_front();

        // Pasar los datos a los parámetros pasados como referencia por si se necesitan usar
        if (name) *name = pkt.socket_name;
        if (port) *port = pkt.port;

        // Devolver los datos
        return pkt.data_rcv;
    }

    std::vector<char> NetMgr::getDataFromSocket(std::string const& socketname) {
        // Puntero al socket objetivo
        std::shared_ptr<UdpSocket> target;

        if (socketname.empty()) {
            SYS_WARN("NetMgr","getDataFromSocket: No name provided.");
            return {};
        }

        // Si hay un socket con ese nombre, pide los datos
        {
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->name() == socketname) { target = sock; break; }
        }

        // Llegará aquí si no encuentra socket
        if (!target) {
            SYS_WARN("NetMgr","Socket not exists with name " + socketname);
            return {};
        }

        // Si no tiene función inyectada es que sus datos se almacenan en su cola interna
        if (!target->hasReceiveCalback())
            return target->getFirstPacket();  // <-- BLOQUEANTE

        // Si tiene función inyectada, los datos estarán el la cola general de NetMgr
        else {
            // Buscamos en la cola centralizada usando el nombre (BLOQUEANTE)
            std::vector<char> data = extractPacketIf([&socketname](const NetPacket& pkt) {
                return pkt.socket_name == socketname;
            });

            if (data.empty()) 
                SYS_WARN("NetMgr", "getDataFromSocket: No packet found for socket " + socketname);
            
            return data;
        }

    }

    std::vector<char> NetMgr::getDataFromSocket(unsigned short local_port) {
        // Puntero al socket objetivo
        std::shared_ptr<UdpSocket> target;

        if (local_port == 0) {
            SYS_WARN("NetMgr","getDataFromSocket: Invalid port:" + std::to_string(local_port));
            return {};
        }

        // Si hay un socket con ese puerto, pide los datos
        {
            std::lock_guard<std::mutex> lock(udp_sockets_mtx_);
            for (const auto& sock : pimpl_->udp_sockets_)
                if (sock->port() == local_port) { target = sock; break; }
        }

        // Llegará aquí si no encuentra socket
        if (!target) {
            
            SYS_WARN("NetMgr","Socket not exists with port " + std::to_string(local_port));
            return {};
        }

        // Si no tiene función inyectada es que sus datos se almacenan en su cola interna
        if(!target->hasReceiveCalback())
            return target->getFirstPacket();  // <-- BLOQUEANTE

        // Si tiene función inyectada, los datos estarán el la cola general de NetMgr
        else {
            // Buscamos en la cola centralizada usando el puerto (BLOQUEANTE)
            std::vector<char> data = extractPacketIf([local_port](const NetPacket& pkt) {
                return pkt.port == local_port;
            });

            if (data.empty()) 
                SYS_WARN("NetMgr", "getDataFromPort: No packet found for port " + std::to_string(local_port));

            return data;
        }
    }

    size_t NetMgr::numUdpRcvElements() {
        return pimpl_->udp_rcv_data_.size();
    }


    // Datos de los sockets guardados -------------------------------------------------------

    int NetMgr::getSocketIndex(short port) const {
        for (unsigned int i = 0; i < pimpl_->udp_sockets_.size(); i++)
            if (pimpl_->udp_sockets_[i]->port() == port) return i;
        /*else*/ return -1;
    }

    int NetMgr::getSocketIndex(std::string const& name) const {
        for (unsigned int i = 0; i < pimpl_->udp_sockets_.size(); i++)
            if (pimpl_->udp_sockets_[i]->name() == name) return i;
        /*else*/ return -1;
    }

    template <typename Lambda>
    std::vector<char> NetMgr::extractPacketIf(Lambda pred) {
        std::unique_lock<std::mutex> lock(udp_rcv_data_mtx_);

        // Esperar hasta tener un paquete válido o hasta que los sockets se paren
        udp_rcv_data_cv_.wait(lock, [this, &pred] {
            if (!sockets_running_) return true; 
            return std::any_of(pimpl_->udp_rcv_data_.begin(), pimpl_->udp_rcv_data_.end(), pred);
        });

        // Buscar si existe el elemento en la cola
        auto it = std::find_if(pimpl_->udp_rcv_data_.begin(), pimpl_->udp_rcv_data_.end(), pred);
        
        // Si lo encuentra, lo extrae
        if (it != pimpl_->udp_rcv_data_.end()) {
            std::vector<char> data = std::move(it->data_rcv);
            pimpl_->udp_rcv_data_.erase(it);
            return data;
        }

        // Si no lo encuentra (porque despertó por detención de red), devuelve vacío
        return {};
    }


    // Operaciones privadas con sockets -----------------------------------------------------
    
    bool NetMgr::RemoveUdpSocketLocked(int index) {
        // Busca el índice del socket en el vector, -1 si falla
        if (index < 0) {
            SYS_WARN("NetMgr","Cannot delete selected UDP socket: Negative vector index.");
            return false;
        }

        // Copiamos el shared_ptr para mantenerlo vivo durante esta función
        std::shared_ptr<UdpSocket> sock = pimpl_->udp_sockets_[index];
        
        // info
        SYS_INFO("NetMgr",
            "Deleting socket '" + sock->name()
            + "' with port " + std::to_string(sock->port() )
        );
                
        // Esto fuerza a que el lambda de async_receive_from se ejecute con error 'operation_aborted'.
        sock->stop();

        // Borrar el elemento del vector usando iterator
        pimpl_->udp_sockets_.erase(pimpl_->udp_sockets_.begin() + index);

        // info y salir
        SYS_INFO("NetMgr","Socket deleted");
        return true;
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct NetMgr::Impl {};


// General ------------------------------------------------------------------------------
    NetMgr::NetMgr(std::size_t const&) {
		SYS_WARN("NetMgr", "Network library has not been implemented.");
    }
    NetMgr::~NetMgr()                               { }

// Gestión de sockets -------------------------------------------------------------------
    bool NetMgr::addUdpSocket(
        std::string         ,
        unsigned short      , 
        const std::string&  , 
        unsigned int        
    ) { return false; }
    bool NetMgr::removeUdpSocket(std::string const&)    { return false; }
    bool NetMgr::removeUdpSocket(unsigned int)          { return false; }
    void NetMgr::printUdpSockets() const                { return; }
    bool NetMgr::socketExists(std::string const&)       { return false; }
    bool NetMgr::socketExists(unsigned short)           { return false; }

// Ejecución ----------------------------------------------------------------------------
    bool NetMgr::start()                                { return false; }
    void NetMgr::stop()                                 { return; }
    bool NetMgr::isRunning() const                      { return false; }
    
// Envío --------------------------------------------------------------------------------
    bool NetMgr::sendData(
        std::string              , 
        const std::vector<char>& ,
        const std::string&       ,
        unsigned short           
    ) { return false; }
    bool NetMgr::sendData(
        unsigned short           , 
        const std::vector<char>& ,
        const std::string&       ,
        unsigned short           
    ) { return false; }

// Recepción ----------------------------------------------------------------------------
    std::vector<char> NetMgr::getNextUdpPacket(std::string*, unsigned short*) { return {}; }
    std::vector<char> NetMgr::getDataFromSocket(std::string const&)           { return {}; }
    std::vector<char> NetMgr::getDataFromSocket(unsigned short)               { return {}; }

// Datos de los sockets guardados -------------------------------------------------------
    int NetMgr::getSocketIndex(std::string const&) const  { return 0; }

#endif