#include "net/UdpSocket.hpp"
#include <system/SystemMgr.hpp>

#if defined ASIO_NETWORK || defined ASIO_NETWORK_VERSION

    #include <asio.hpp>                 // asio external lib

    constexpr std::size_t MAX_UDP_PACKET_SIZE = 65536; // Tamaño máximo de un paquete UDP (64 KB)

    using CStrand = asio::strand<asio::io_context::executor_type>;
    using CSocket = asio::basic_datagram_socket<asio::ip::udp, asio::io_context::executor_type>;

    // Implementación de miembros de la clase de asio (pimpl_)
    struct UdpSocket::Impl {
        CStrand                     strand_;            ///< Protección del buffer asíncrono de recepción
        CSocket                     socket_;            ///< Socket asio

        asio::ip::udp::endpoint     local_endpoint_;    ///< Endpoint local (ip+puerto local)
        asio::ip::udp::endpoint     remote_endpoint_;   ///< Endpoint remoto desde el que se reciben los datos

        // El constructor recibe la referencia del io_context general

        /**
         * @brief Constructor de Impl
         *  El constructor del Impl hace el cast de los punteros opacos 
         * @param io (asio::io_context*) Contexto de red de asio
         */
        Impl(void* io);
    };

    // Implementación de métodos PIMPL ------------------------------------------------------
    
    UdpSocket::Impl::Impl(void* io) :
        strand_(asio::make_strand(*static_cast<asio::io_context*>(io))),
        socket_(*static_cast<asio::io_context*>(io)),
        local_endpoint_(),
        remote_endpoint_()
    {
        
    }


    // General ------------------------------------------------------------------------------

    UdpSocket::UdpSocket(std::string const& name, void* io) :
        pimpl_(std::make_unique<Impl>(io)),
        initialized_(false), 
        running_    (false),
        name_(name), 
        rcv_packet_size_(0),
        has_rcv_packet_(false),
        ignore_dupe_(true)
    {
        if(!io)
            SYS_ERROR("UdpSocket","Initialized socket without network io_context.");
        else
            pimpl_ = std::make_unique<Impl>(io);
    }

    UdpSocket::~UdpSocket() {
        stop();
        //clearQueue();     // <- En el stop() ya se hace clearQueue
    }


    // Ejecución ----------------------------------------------------------------------------

    bool UdpSocket::init(unsigned short local_port, const std::string& local_ip, unsigned int rcv_packet_size) {
        
        std::string st_local_ip = ((local_ip.empty()) ? "all" : local_ip );
        std::string st_rcv_packet_size = (rcv_packet_size==0) ? "any" : std::to_string(rcv_packet_size);

        SYS_INFO("UdpSocket", 
            "Registering new socket: port: " + 
            std::to_string(local_port) +
            ", IP: " + st_local_ip + 
            ", rcv_size: " + st_rcv_packet_size
        );

        // No hacer nada si ya estaba inicializado 
        if (initialized_) 
            return true;

        // Check puerto válido
        if (local_port==0) {
            SYS_WARN("UdpSocket","Port 0 is invalid.");
            return false;
        }

        // Check tamaño de paquete válido
        if (rcv_packet_size > MAX_UDP_PACKET_SIZE) {
            SYS_WARN("UdpSocket","Packet size exceeds maximum UDP packet size.");
            return false;
        }
        
        // Variable para almacenar errores de asio
        asio::error_code ec;
        
        // Redimensionar el buffer de recepción al tamaño esperado de los paquetes, máximo cuando no se especifica
        this->rcv_packet_size_ = rcv_packet_size;
        recv_buffer_.resize(rcv_packet_size_ > 0 ? rcv_packet_size_ : MAX_UDP_PACKET_SIZE);

        // generar local_endpoint (Ip_local + puerto_local)
        if (!create_local_endpoint(local_port, local_ip)) {
            SYS_WARN("UdpSocket","Failed to create endpoint.");
            return false;
        }

        // Abrir (bind) el socket
        if (!open_socket()) {
            if (pimpl_->socket_.is_open()) {
                ec = pimpl_->socket_.close(ec);
                if (ec)
                    SYS_WARN("UdpSocket","Error closing socket after failure: " + ec.message());
            }
            return false;
        }

        SYS_INFO("UdpSocket", "Socket initialized successfully");
        initialized_ = true;
        return true;
    }

    void UdpSocket::start() {
        if (running_) return;
        if (!pimpl_->socket_.is_open() && !open_socket()) {
            SYS_WARN("UdpSocket","Cannot open socket" + name_ + ":" + std::to_string(pimpl_->local_endpoint_.port()));
            return;
        }
        if (!initialized_) {
            SYS_WARN("UdpSocket","Socket " + name_ + ":" + std::to_string(pimpl_->local_endpoint_.port()) + " not initialized.");
            return;
        }

        // Vincular el socket al callback de recepción
        start_receive();

        SYS_INFO("UdpSocket", "Socket " + name_ + ":" + std::to_string(pimpl_->local_endpoint_.port()) + " started successfully");
        running_ = true;
    }

    void UdpSocket::stop() {
        asio::error_code ec;
        
        // Cerrar el socket si está abierto
        if (pimpl_->socket_.is_open()) {
            // Cancelar cualquier operación pendiente
            ec = pimpl_->socket_.cancel(ec);
            if (ec)
                SYS_WARN("UdpSocket","Failed to cancel remaining operations");

            // Cerrar completamente
            ec = pimpl_->socket_.close(ec);
            if (ec)
                SYS_WARN("UdpSocket","Failed to close socket");
        }

        // Despertar cualquier hilo que esté esperando en la cola de datos
        condition_.notify_all(); 

        // Perparar por si hay reinicio
        clear_queue();
        
        running_ = false;
    }


    // Envío --------------------------------------------------------------------------------

    bool UdpSocket::sendPacket(const std::vector<char>& data, const std::string& ip, unsigned short port) {
        // Socket inválido
        if (!pimpl_->socket_.is_open())
        {
            SYS_WARN("UdpSocket","[" + name_ + "] sendPacket() -> socket closed");
            return false;
        }

        // No enviar paquetes vacíos
        if (data.empty())
        {
            SYS_WARN("UdpSocket","[" + name_ + "] sendPacket() -> empty packet");
            return false;
        }

        // Si está en stop/offline, no deja mandar nada
        if(!running_) {
            SYS_WARN("UdpSocket","Cannot send anything. Socket is not running.");
            return false;
        }

        try
        {
            // Crear endpoint remoto
            asio::ip::udp::endpoint endpoint(
                asio::ip::make_address(ip),
                port
            );

            // Enviar datos
            const std::size_t bytes_sent =
                pimpl_->socket_.send_to(asio::buffer(data), endpoint);

            // Verificar envío completo
            if (bytes_sent != data.size())
            {
                SYS_WARN("UdpSocket","[" + name_ + "]   sendPacket() -> incomplete send (" + std::to_string(bytes_sent) + "/" + std::to_string(data.size()) + ")");
                return false;
            }

            SYS_INFO("UdpSocket","[" + name_ + "] sendPacket() -> " + std::to_string(bytes_sent) + " bytes sent successfully.");
            return true;
        }
        catch (const std::exception& e)
        {
            SYS_WARN("UdpSocket","[" + name_ + "] sendPacket() exception: " + e.what());
            return false;
        }
    }


    // Recepción ----------------------------------------------------------------------------

    std::vector<char> UdpSocket::getFirstPacket() {

        if (!running_) {
            SYS_WARN("UdpSocket", "getFirstPacket: Socket is not running.");
            return {};
        }

        std::unique_lock<std::mutex> lock(mutex_);
        // Se BLOQUEA aquí hasta que la cola no esté vacía
        condition_.wait(lock, [this] { 
                return !queue_.empty() || !pimpl_->socket_.is_open() || !running_; 
            }
        );

        // En parada del socket devolvemos vector vacío
        if (queue_.empty() || !running_) return {};

        std::vector<char> data = std::move(queue_.front());
        queue_.pop();
        return data;
    }

    void UdpSocket::discardOnDupe(bool enable){
        ignore_dupe_ = enable;
    }

    void UdpSocket::clearRcvCache() {
        clear_queue();
    }

    bool UdpSocket::hasData() {
        return !is_queue_empty();
    }

    unsigned long long UdpSocket::getLastPacketMs() const {
        if (!has_rcv_packet_) 
            return 0; 

        // Tomamos el tiempo actual
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        // Calculamos la diferencia
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_packet_time_);

        // Devolvemos el conteo en milisegundos
        return duration.count();
    }

    void* UdpSocket::getStrandNative() { 
        return static_cast<void*>(&pimpl_->strand_);
    }

    // Callback de recepción ----------------------------------------------------------------

    void UdpSocket::setCallback_onReceive(std::function<void(NetPacket)> cb) {
        std::lock_guard<std::mutex> lk(onReceive_mtx_);
        onReceive_cb_ = std::move(cb); 
    }

    void UdpSocket::clearCallback_onReceive() {
        std::lock_guard<std::mutex> lk(onReceive_mtx_);
        onReceive_cb_ = nullptr;
    }

    bool UdpSocket::hasCallback_onReceive() const {
        std::lock_guard<std::mutex> lk(onReceive_mtx_);
        return static_cast<bool>(onReceive_cb_);
    }


    // Datos de socket ----------------------------------------------------------------------

    unsigned short UdpSocket::port() const {
        if (pimpl_->socket_.is_open())  return pimpl_->socket_.local_endpoint().port();
        else                    return 0; // Indica que el socket no está abierto
    }

    std::string const& UdpSocket::name() const {
        return name_;
    }

    bool UdpSocket::isRunning() const {
        return running_;
    }

    bool UdpSocket::isOpen() const {
        return pimpl_->socket_.is_open();
    }


    // Estadísticas / diagnóstico ---------------------------------------------------------

    // Devuelve una FOTO de cómo va el socket ahora mismo, para pintarla en la ventana
    // "Network Checking". La GUI llama a esto; no puede leer las variables internas
    // directamente (son privadas y las cambia el hilo de red a la vez).
    //
    // Cómo funciona: crea una caja 'Stats' vacía, copia dentro cada dato, y la devuelve.
    // Al ser una copia, la GUI la pinta tranquila aunque justo después lleguen más paquetes.
    UdpSocket::Stats UdpSocket::stats() const {

        Stats s;   // caja vacía que iremos rellenando

        // --- Datos que van con 'atomic' (números): se copian directamente ---
        s.running       = running_;            
        s.rx_count      = rx_count_;           
        s.period_ms     = last_period_ms_;     
        s.last_size     = last_rx_bytes_;      

        // --- Datos que ya calculaban otras funciones del socket ---
        s.since_last_ms = getLastPacketMs();     // ms desde el último paquete (0 si nunca llegó ninguno)
        s.expected_size = rcv_packet_size_;      // tamaño que se espera (0 = cualquiera) -> "Size"
        s.local_ip      = pimpl_->local_endpoint_.address().to_string();  // IP local -> "LOCAL IP"
        s.local_port    = port();                // puerto local                 -> "PORT"

        // --- IP del emisor: es texto, lo copiamos con el candado echado ---
        // (así no lo leemos a medias mientras el hilo de red lo está escribiendo)
        {
            std::lock_guard<std::mutex> lk(stats_mtx_);
            s.remote_ip   = remote_ip_;          // quién mandó el último paquete -> "IP FROM"
            s.remote_port = remote_port_;
        }

        return s;   // entregamos la foto
    }

    // Creación de socket -------------------------------------------------------------------

    bool UdpSocket::create_local_endpoint(unsigned short local_port, const std::string& local_ip) {

        // Variable para almacenar errores de asio
        asio::error_code ec;

        // Si la ip local es vacía, se enlaza a todas las interfaces
        if (local_ip.empty()) {
            pimpl_->local_endpoint_ = asio::ip::udp::endpoint(asio::ip::udp::v4(), local_port);
        }
        // Si se especifica una IP local, se enlaza a esa IP
        else {
            // Convertir la cadena a asio::ip::address
            asio::ip::address localIP = asio::ip::make_address(local_ip, ec);
            if (ec) {
                SYS_WARN("UdpSocket","Invalid IP: " + local_ip + " - " + ec.message());
                return false;
            }
            SYS_INFO("UdpSocket","Creating socket " + local_ip + ":" + std::to_string(local_port));
            pimpl_->local_endpoint_ = asio::ip::udp::endpoint(localIP, local_port);
        }
        return true;
    }

    bool UdpSocket::open_socket() {

        // Variable para almacenar errores de asio
        asio::error_code ec;

        // Abrir el socket 
        ec.clear();
        ec = pimpl_->socket_.open(pimpl_->local_endpoint_.protocol(), ec);
        if (ec) {
            SYS_WARN("UdpSocket", "Failed opening socket: " + ec.message());
            return false;
        }

        // Confirgurar el socket para permitir reutilizar la dirección local
        ec.clear();
        ec = pimpl_->socket_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            SYS_WARN("UdpSocket","Error setting socket option: " + ec.message());
            return false;
        }

        // Enlazar el socket al endpoint local
        ec.clear();
        ec = pimpl_->socket_.bind(pimpl_->local_endpoint_, ec);
        if (ec) {
            if (ec == asio::error::access_denied)
                SYS_WARN("UdpSocket","Access denied binding socket. Check firewall rules and try again.");
            else if (ec == asio::error::address_in_use)
                SYS_WARN("UdpSocket","Port " + std::to_string(pimpl_->local_endpoint_.port()) + " already in use.");
            else
                SYS_WARN("UdpSocket","Unhandled error: " + ec.message());
            return false;
        }

        return true;
    }


    // Callback de recepción ----------------------------------------------------------------

    void UdpSocket::start_receive() {
        std::shared_ptr<UdpSocket> self(shared_from_this());  // Mantiene el objeto vivo si se destruye antes

        pimpl_->socket_.async_receive_from(
            asio::buffer(recv_buffer_),
            pimpl_->remote_endpoint_,
            asio::bind_executor(pimpl_->strand_, [this, self](std::error_code ec, std::size_t bytes_recvd) {

                if (!running_) return;

                // Gestionar los errores si hay
                if (ec) {
                    if (ec == asio::error::operation_aborted) {
                        // Caso esperado al cerrar el socket o borrar el receptor
                        SYS_INFO("UdpSocket", "Socket '" + name_ + "' stopped successfully");
                        return;
                    } else {
                        // Continúa recibiendo a pesar del error (si !abortar)
                        SYS_WARN("UdpSocket","Socket "+name_+"Reception error: " + ec.message());
                        if (pimpl_->socket_.is_open())
                            start_receive();
                    }
                }

                else {
                    // Pasamos a procesar el paquete
                    handle_received_packet(bytes_recvd);
                    // Pedimos el siguiente paquete SÓLO cuando terminamos con este
                    this->start_receive();
                }
            })
        );
    }

    void UdpSocket::handle_received_packet(std::size_t bytes_recvd) {
        
        // Si se ha recibido un paquete vacío, lo reportamos pero seguimos esperando
        if(bytes_recvd == 0) {
            SYS_WARN("UdpSocket","Received empty packet."); // no sé meter remote_endpoint_.address()
            return;
        }

        // Si se ha especificado un tamaño esperado de paquete y el tamaño recibido es diferente, lo reportamos pero seguimos esperando
        if (rcv_packet_size_!=0 && bytes_recvd!=rcv_packet_size_){
            SYS_WARN("UdpSocket","Received packet size different from expected (" +
                std::to_string(bytes_recvd) + " bytes, expected " + std::to_string(rcv_packet_size_) + " bytes)");
            return;
        }

        // Si el paquete ya no entra en la cola, lo descartamos
        if (is_queue_full()){
            SYS_WARN("UdpSocket","Reception queue overloaded with " + std::to_string(get_queue_size()) + " elements");
            return;
        }
        
        // Redimensiona el paquete al tamaño que se espera recibir 
        std::vector<char> data(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd);
        
        // Descarta el paquete si es igual que el anterior (si está activada esta opción)
        if(ignore_dupe_ && compare_last(data) ) {
            SYS_INFO("UdpSocket", "Received packet same as last. Ignoring...");
            return;
        }

        // Estadísticas para la ventana "Network Checking"
        // Llegados aquí, el paquete es válido. Guardamos unos cuantos datos que luego la GUI leerá con stats().

        auto now = std::chrono::steady_clock::now();// Hora de ahora (cronómetro interno del sistema)

        // Si YA había llegado algún paquete antes -> cuánto tiempo ha pasado desde ese hasta este.
        // La primera vez no se calcula porque no hay un "paquete anterior" con el que comparar.
        if (has_rcv_packet_)
            last_period_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_packet_time_).count();

        last_packet_time_ = now;             // este paquete pasa a ser "el último"
        has_rcv_packet_   = true;            // a partir de ahora sí hay un "paquete anterior"
        ++rx_count_;                         // un paquete más -> este contador es el "Cicle"
        last_rx_bytes_    = bytes_recvd;     // tamaño real de este paquete -> "Real size"

        // De quién vino (IP + puerto del emisor).
        {
            std::lock_guard<std::mutex> lk(stats_mtx_);
            remote_ip_   = pimpl_->remote_endpoint_.address().to_string();   // -> "IP FROM"
            remote_port_ = pimpl_->remote_endpoint_.port();
        }

        // Añade el paquete recibido a la cola listo para gestionar
        if (onReceive_cb_) {
            // Si NetMgr (o alguien) registró un callback, le pasamos los datos
            // Usamos std::move para no copiar el vector en memoria
            NetPacket packet;
            packet.socket_name = name_;
            packet.port = port(); // Usamos la función port() de tu clase
            packet.data_rcv = std::move(data);
            
            // Pasamos el struct entero al callback
            onReceive_cb_(std::move(packet));
        } else {
            // Si el socket se usa de forma independiente, usa su propia cola local
            save_to_queue(std::move(data)); 
        }
    }

    
    // Gestión de cola de datos recibidos ---------------------------------------------------

    void UdpSocket::save_to_queue(std::vector<char> data) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(data));
        condition_.notify_one(); // Avisa que hay datos
    }

    bool UdpSocket::is_queue_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    bool UdpSocket::is_queue_full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return (queue_.size() >= MAX_QUEUE_ELEMENTS);
    }

    size_t UdpSocket::get_queue_size() const{
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void UdpSocket::clear_queue() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_ = std::queue<std::vector<char>>{};
    }

    bool UdpSocket::compare_last(std::vector<char> const& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Devuelve false si la cola está vacía
        if (queue_.empty()) 
            return false;

        // Devuelve si es igual que el anterior
        return queue_.back() == data;
    }

#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct UdpSocket::Impl {};


// General ------------------------------------------------------------------------------
    UdpSocket::UdpSocket(std::string const&, void*) {
		SYS_WARN("UdpSocket", "Network library has not been implemented.");
    }
    UdpSocket::~UdpSocket() { }
    
// Ejecución ----------------------------------------------------------------------------
    bool UdpSocket::init(unsigned short, const std::string&, unsigned int) { return false;}
    void UdpSocket::start() { }
    void UdpSocket::stop()  { }

// Envío --------------------------------------------------------------------------------
    bool UdpSocket::sendPacket(const std::vector<char>&, const std::string&, unsigned short) { return false;}

// Recepción ----------------------------------------------------------------------------
    std::vector<char> UdpSocket::getFirstPacket()  { return {};     }
    void UdpSocket::setCallback_onReceive(std::function<void(NetPacket)> cb) { return; }
    void UdpSocket::clearCallback_onReceive()      { return;        }
    bool UdpSocket::hasCallback_onReceive() const  { return false;  }
    void UdpSocket::discardOnDupe(bool)            { return;        }
    bool UdpSocket::hasData()                      { return false;  }
    unsigned long long UdpSocket::getLastPacketMs() const   { return 0;      }
    void* UdpSocket::getStrandNative()             { return nullptr;}

// Datos de socket ----------------------------------------------------------------------
    unsigned short UdpSocket::port() const         { return 0;     }
    std::string const& UdpSocket::name() const     { return name_; }
    bool UdpSocket::isRunning() const              { return false; }
    bool UdpSocket::isOpen() const                 { return false; }

// Estadísticas / diagnóstico -----------------------------------------------------------
    UdpSocket::Stats UdpSocket::stats() const { return {}; }

// Creación de socket -------------------------------------------------------------------
    bool UdpSocket::create_local_endpoint(unsigned short, const std::string&) { return false;}
    bool UdpSocket::open_socket() { return false; }

// Callback de recepción ----------------------------------------------------------------
    void UdpSocket::start_receive()                        { return; }
    void UdpSocket::handle_received_packet(std::size_t)    { return; }

// Gestión de la cola de datos recibidos ------------------------------------------------
    void UdpSocket::save_to_queue(std::vector<char>)        { return;       }
    bool UdpSocket::is_queue_empty() const                  { return false; }
    bool UdpSocket::is_queue_full() const                   { return false; }
    size_t UdpSocket::get_queue_size() const                { return 0;     }
    void UdpSocket::clear_queue()                           { return;       }
    bool UdpSocket::compare_last(std::vector<char> const&)  { return false; }

#endif
