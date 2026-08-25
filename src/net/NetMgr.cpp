#include "net/NetMgr.hpp"
#include "system/SystemMgr.hpp"

#if defined ASIO_NETWORK || defined ASIO_NETWORK_VERSION

    #include <asio.hpp>                 // asio external lib
    #include "net/UdpSocket.hpp"        // Para conocer UdpSocket
    #include "datatypes/netTypes.hpp"   // Para conocer NetPacket
    #include "files/JsonMgr.hpp"        // Para conocer json


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
        running_(false),
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
        // Si ya está inicializado no hacer nada
        if (initialized_)
            return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

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

        // Activar running para los hilos
        running_ = true;

        // Hilo consumidor de paquetes online (si net activo)
        SYS_INFO("NetMgr","Starting dispatcher thread...");
        dispatcher_thread_ = std::thread(&NetMgr::t_dispatcher, this);

        SYS_INFO("NetMgr","Sockets running");
        sockets_running_    = true;
        running_            = true;            // Para hilo consumidor
        initialized_        = true;
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

        sockets_running_    = true;
        return true;
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
        dispatcher_cv_.notify_all();   // desbloquea getNextPacket()
        SYS_INFO("NetMgr","All sockets stopped");
    }

    bool NetMgr::close() {
        // Parar los sockets
        stop();

        // Parar flag para hilos
        running_ = false;

        // Soltar el work_guard para que io_context pueda drenar y terminar.
        pimpl_->work_guard_.reset();

        // Esperar a que los hilos salgan naturalmente (sin stop() forzado).
        //    io_context terminará solo cuando no queden handlers pendientes.
        for (auto& t : threads_)
            if (t.joinable()) t.join(); // Esperar a los hilos de io_context
        io_running_ = false;
        threads_.clear();
    
        // Cerrar hilos pendientes de aplicación
        dispatcher_cv_.notify_all();
        if (dispatcher_thread_.joinable()) {
            SYS_INFO("AppController","Waiting for consumer thread...");
            dispatcher_thread_.join();
        }

        initialized_ = false;
        return true;
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
        socket->setCallback_onReceive([this](NetPacket packet) {

            // Proteger la cola de datos del socket
            std::lock_guard<std::mutex> lock(udp_rcv_data_mtx_);

            // Agregar dato recibido a la cola general de NetMgr (si cabe)
            if (pimpl_->udp_rcv_data_.size() < MAX_QUEUE_ELEMENTS_)
                pimpl_->udp_rcv_data_.push_back(std::move(packet));
            else
                SYS_WARN("NetMgr", "Central queue full, dropping packet from " + packet.socket_name + ":" + std::to_string(packet.port));

            // Notificar 
            dispatcher_cv_.notify_all();
        });

        // Intentar inicializar
        SYS_INFO("NetMgr","Opening new socket...");
        if (!socket->init(local_port, local_ip, rcv_packet_size))
        {
            // No hay nada que limpiar, el puntero make_unique se destruye al salir.
            SYS_WARN("NetMgr","Failed to initialize socket on port " + std::to_string(local_port));
            return false;
        }

        // Insertar en el vector
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
        return remove_udp_socket_locked(get_socket_index(name));
    }

    bool NetMgr::removeUdpSocket(unsigned int port) {
        // Protege el vector de sockets de otras llamadas
        std::lock_guard<std::mutex> lock(udp_sockets_mtx_);

        // Llama función privada con lock adquirido
        return remove_udp_socket_locked(get_socket_index(port));
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

    size_t NetMgr::numUdpRcvElements() {
        return pimpl_->udp_rcv_data_.size();
    }


    // Dispatcher ----------------------------------------------------------------------------

    void NetMgr::t_dispatcher() {

        while (running_) {
            NetPacket packet;               // Estructura de datos recibidos
            std::vector<char> data = {};    // Datos recibidos (de la estructura)
            
            // Esperar y obtener datos de la cola
            {
                // Forzar la espera hasta que sea notificado de un paquete nuevo
                std::unique_lock<std::mutex> lock(udp_rcv_data_mtx_);
                dispatcher_cv_.wait(lock, [this] {
                    return !running_ || !pimpl_->udp_rcv_data_.empty();
                });

                // Salir si el programa se está cerrando
                if (!running_) 
                    break;

                // Obtener los datos de la cola
                packet = std::move(pimpl_->udp_rcv_data_.front());
                pimpl_->udp_rcv_data_.pop_front();
                data = packet.data_rcv;
            }

            // Si la cola está vacía no hacer nada
            if (pimpl_->udp_rcv_data_.empty())
                continue;

            // Mostrar info del paquete recibido
            data = packet.data_rcv;
            std::string pktSize = std::to_string(packet.data_rcv.size());
            SYS_INFO("NetMgr",
                "UDP Packet from " +  packet.socket_name + ":" + std::to_string(packet.port) +
                " (size: " + pktSize + ")" 
            );

            // Salir si el programa se está cerrando (después de getpacket)
            if (!running_) break;

            // Si no hay datos no hacer nada
            if (packet.data_rcv.empty()) {
                SYS_WARN("TWorker","Empty data received");
                continue;
            }

            // Procesar el paquete (simulado)
            SYS_INFO("NetMgr", "Procesando paquete de datos...");
            SYS_INFO("NetMgr","Size of data " + std::to_string(data.size()));
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        SYS_INFO("NetMgr", "Consumer thread stopped.");
    }


    // Datos de los sockets guardados -------------------------------------------------------

    int NetMgr::get_socket_index(short port) const {
        for (unsigned int i = 0; i < pimpl_->udp_sockets_.size(); i++)
            if (pimpl_->udp_sockets_[i]->port() == port) return i;
        /*else*/ return -1;
    }

    int NetMgr::get_socket_index(std::string const& name) const {
        for (unsigned int i = 0; i < pimpl_->udp_sockets_.size(); i++)
            if (pimpl_->udp_sockets_[i]->name() == name) return i;
        /*else*/ return -1;
    }


    // Operaciones privadas con sockets -----------------------------------------------------
    
    bool NetMgr::remove_udp_socket_locked(int index) {
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
    bool NetMgr::socketExists(std::string const&) const { return false; }
    bool NetMgr::socketExists(unsigned short) const     { return false; }

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
    int NetMgr::get_socket_index(short port) const          { return 0; }
    int NetMgr::get_socket_index(std::string const&) const  { return 0; }
    
    template <typename Lambda>
    std::vector<char> extract_packet_if(Lambda)             { return {}; }

// Operaciones privadas con sockets -----------------------------------------------------
    bool remove_udp_socket_locked(int)                      { return false; }

#endif
