#include "net/UdpReceiver.hpp"
#include <iostream>
#include <atomic>

#define MAX_UDP_PACKET_SIZE 65536 // Tamaño máximo de un paquete UDP (64 KB)

// General ------------------------------------------------------------------------------

UdpReceiver::UdpReceiver(std::string name, asio::io_context& io)
    : name_(name), 
    strand_(asio::make_strand(io)), 
    socket_(io), 
    rcv_packet_size_(0), 
    initialized_(false), 
    running_(false),
    ignore_dupe_(true)
{
	
}

UdpReceiver::~UdpReceiver() {
    stop();
    clearQueue();
}

bool UdpReceiver::init(unsigned short local_port, const std::string& local_ip, unsigned int rcv_packet_size) {
    
    std::cout << "[UdpReceiver] Creating new socket..." << std::endl;
    std::cout << "[UdpReceiver]     port: "     << local_port       << std::endl;
    std::cout << "[UdpReceiver]     IP: "       << ((local_ip=="") ? "all" : local_ip )       << std::endl;
    std::cout << "[UdpReceiver]     rcv_size: " << ((rcv_packet_size==0) ? MAX_UDP_PACKET_SIZE : rcv_packet_size ) << std::endl;

    // No hacer nada si ya estaba inicializado 
    if (initialized_) 
        return true;

    // Check puerto válido
    if (local_port==0) {
        std::cerr << "[UdpReceiver] ERROR Port 0 is invalid." << std::endl;
        return false;
    }

    // Check tamaño de paquete válido
    if (rcv_packet_size > MAX_UDP_PACKET_SIZE) {
        std::cerr << "[UdpReceiver] Packet size exceeds maximum UDP packet size." << std::endl;
        return false;
    }

    
    // Variable para almacenar errores de asio
    asio::error_code ec;
    
    // Redimensionar el buffer de recepción al tamaño esperado de los paquetes
    this->rcv_packet_size_ = rcv_packet_size;
    recv_buffer_.resize(rcv_packet_size_ > 0 ? rcv_packet_size_ : MAX_UDP_PACKET_SIZE);

    // generar local_endpoint (Ip_local + puerto_local)
    if (!createLocalEndpoint(local_port, local_ip)) {
        std::cerr << "[UdpReceiver] ERROR Failed to create endpoint." << std::endl;
        return false;
    }

    // Abrir (bind) el socket
    if (!openSocket() && socket_.is_open()) {
        std::cerr << "[UdpReceiver] ERROR Socket is not open after bind." << std::endl;
        socket_.close(ec);
        if (ec) std::cerr << "[UdpReceiver] ERROR closing socket after failure: " << ec.message() << std::endl;
        return false;
    }

    std::cout << "[UdpReceiver] Socket initialized successfully" << std::endl;
    initialized_ = true;
    return true;
}

bool UdpReceiver::createLocalEndpoint(unsigned short local_port, const std::string& local_ip) {

    // Variable para almacenar errores de asio
    asio::error_code ec;

    // Si la ip local es vacía, se enlaza a todas las interfaces
    if (local_ip.empty()) {
        local_endpoint_ = asio::ip::udp::endpoint(asio::ip::udp::v4(), local_port);
    }
    // Si se especifica una IP local, se enlaza a esa IP
    else {
        // Convertir la cadena a asio::ip::address
        asio::ip::address localIP = asio::ip::make_address(local_ip, ec);
        if (ec) {
            std::cerr << "[UdpReceiver] ERROR Invalid IP: " << local_ip << " - " << ec.message() << std::endl;
            return false;
        } 
        local_endpoint_ = asio::ip::udp::endpoint(localIP, local_port);
    }
    return true;
}

void UdpReceiver::stop() {
    asio::error_code ec;
    
    // Cerrar el socket si está abierto
    if (socket_.is_open()) {
        socket_.cancel(ec); // Cancelar cualquier operación pendiente
        socket_.close(ec);
    }
    if (ec) std::cerr << "[UdpReceiver] ERROR Failed closing socket: " << ec.message() << std::endl;

    // Despertar cualquier hilo que esté esperando en la cola de datos
    condition_.notify_all(); 

    // Perparar por si hay reinicio
    clearQueue();
    
    running_ = false;
}

short UdpReceiver::port() const {
    if (socket_.is_open())  return socket_.local_endpoint().port();
    else                    return -1; // Indica que el socket no está abierto
}

std::string const& UdpReceiver::name() const {
    return name_;
}

bool UdpReceiver::isRunning() const {
    return running_;
}

bool UdpReceiver::isOpen() const {
    return socket_.is_open();
}


bool UdpReceiver::openSocket() {

    // Variable para almacenar errores de asio
    asio::error_code ec;

    // Abrir el socket 
    socket_.open(local_endpoint_.protocol(), ec);
    if (ec) {
        std::cerr << "[UdpReceiver] ERROR Failed opening socket: " << ec.message() << std::endl;
        return false;
    }
    
    // Confirgurar el socket para permitir reutilizar la dirección local 
    socket_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "[UdpReceiver] ERROR setting socket option: " << ec.message() << std::endl;
        return false;
    }

    // Enlazar el socket al endpoint local
    socket_.bind(local_endpoint_, ec);
    if (ec) {
        if (ec == asio::error::access_denied) 
            std::cerr << "[UdpReceiver] ERROR Access denied. Check firewall rules and try again." << std::endl;
        else if (ec == asio::error::address_in_use) 
            std::cerr << "[UdpReceiver] ERROR Port " << local_endpoint_.port() << " already in use." << std::endl;
        else
            std::cerr << "[UdpReceiver] ERROR unhandled error: " << ec.message() << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void UdpReceiver::start() {
    if (running_)           return;
    if (!socket_.is_open() && !openSocket()) {
        std::cerr << "[UdpReceiver] ERROR Cannot open socket" << name_ << ":"<< local_endpoint_.port() << std::endl;
        return;
    }
    if (!initialized_) {
        std::cerr << "[UdpReceiver] ERROR socket " << name_ << ":" << local_endpoint_.port() <<" not initialized." << std::endl;
        return;
    }

    start_receive();

    std::cout << "[UdpReceiver] Socket " << name_ << ":" << local_endpoint_.port() <<" started successfully" << std::endl;
    running_ = true;
}

void UdpReceiver::start_receive() {
    std::shared_ptr<UdpReceiver> self(shared_from_this());  // Mantiene el objeto vivo si se destruye antes

    socket_.async_receive_from(
        asio::buffer(recv_buffer_),
        remote_endpoint_,
        asio::bind_executor(strand_, [this, self](std::error_code ec, std::size_t bytes_recvd) {

            if (!running_) return;

            // Gestionar los errores si hay
            if (ec) {
                if (ec == asio::error::operation_aborted) {
                    // Caso esperado al cerrar el socket o borrar el receptor
                    std::cout << "[UdpReceiver] Socket " << name_ << " stopped successfully." << std::endl;
                    return;
                } else {
                    // Continúa recibiendo a pesar del error (si !abortar)
                    std::cerr << "[UdpReceiver] "<< name_ << " ERROR Reception error: " << ec.message() << std::endl;
                    if (socket_.is_open())
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

void UdpReceiver::handle_received_packet(std::size_t bytes_recvd) {
    
    // Si se ha recibido un paquete vacío, lo reportamos pero seguimos esperando
    if(bytes_recvd == 0) {
        std::cerr << "[UdpReceiver] WARN Received empty packet from " << remote_endpoint_ << std::endl;
        return;
    }

    // Si se ha especificado un tamaño esperado de paquete y el tamaño recibido es diferente, lo reportamos pero seguimos esperando
    if (rcv_packet_size_!=0 && bytes_recvd!=rcv_packet_size_){
        std::cerr << "[UdpReceiver] WARN Received packet size different from expected (" << bytes_recvd << " bytes, expected " << rcv_packet_size_ << " bytes) from " << remote_endpoint_ << std::endl;
        return;
    }

    // Si el paquete ya no entra en la cola, lo descartamos
    if (isQueueFull()){
        std::cerr << "[UdpReceiver] WARN Reception queue overloaded with " << getQueueSize() << " elements" << std::endl;
        return;
    }
    
    // Redimensiona el paquete al tamaño que se espera recibir 
    std::vector<char> data(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd);
    
    // Descarta el paquete si es igual que el anterior (si está activada esta opción)
    if(ignore_dupe_ && compareLast(data) ) {
        std::cout << "[UdpReceiver] Received packet same as last. Ignoring..." << std::endl;
        return;
    }

    // Añade el paquete recibido a la cola listo para gestionar
    savePacket(std::move( data ) ); 
}


// Gestión de cola de datos ------------------------------------------------------------

std::vector<char> UdpReceiver::getFirstPacket() {
    std::unique_lock<std::mutex> lock(mutex_);
    // Se BLOQUEA aquí hasta que la cola no esté vacía
    condition_.wait(lock, 
        [this] { 
            return !queue_.empty() || !socket_.is_open(); 
        }
    );

    // En parada del socket devolvemos vector vacío
    if (queue_.empty()) return {};

    std::vector<char> data = std::move(queue_.front());
    queue_.pop();
    return data;
}

void UdpReceiver::discardOnDupe(bool enable){
    ignore_dupe_ = enable;
}

void UdpReceiver::clearCache() {
    clearQueue();
}

bool UdpReceiver::hasData() {
	return !isQueueEmpty();
}


void UdpReceiver::savePacket(std::vector<char> data) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(data));
    condition_.notify_one(); // Avisa que hay datos
}

bool UdpReceiver::isQueueEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

bool UdpReceiver::isQueueFull() const{
    std::lock_guard<std::mutex> lock(mutex_);
    return (queue_.size()>=MAX_QUEUE_ELEMENTS);
}

size_t UdpReceiver::getQueueSize() const{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void UdpReceiver::clearQueue(){
    std::lock_guard<std::mutex> lock(mutex_);
    queue_ = std::queue<std::vector<char>>{};
}

bool UdpReceiver::compareLast(std::vector<char> const& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Devuelve false si la cola está vacía
    if (queue_.empty()) 
        return false;

    // Devuelve si es igual que el anterior
    return queue_.back() == data;
}
