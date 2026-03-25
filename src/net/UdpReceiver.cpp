#include "net/UdpReceiver.hpp"
#include <iostream>
#include <atomic>

#define MAX_UDP_PACKET_SIZE 65536 // Tamaño máximo de un paquete UDP (64 KB)

// General ------------------------------------------------------------------------------

UdpReceiver::UdpReceiver(std::string name, asio::io_context& io)
    : name_(name), strand_(asio::make_strand(io)), socket_(io), rcv_packet_size_(0), ignore_dupe_(true)
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

    // No hacer nada si ya existía 
    if (isRunning()) 
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
    
    // Redimensionar el buffer de recepción al tamaño esperado de los paquetes
    this->rcv_packet_size_ = rcv_packet_size;
    recv_buffer_.resize(rcv_packet_size_ > 0 ? rcv_packet_size_ : MAX_UDP_PACKET_SIZE);

    // Abrir (bind) el socket
    if (openSocket(local_port, local_ip) && socket_.is_open()) {
        std::cout << "[UdpReceiver] Registering callback function..." << std::endl;
        start_receive();
    }
    else {
        std::cerr << "[UdpReceiver] ERROR Socket is not open after bind." << std::endl;
        asio::error_code ec;
        socket_.close(ec);
        if (ec) std::cerr << "[UdpReceiver] ERROR closing socket after failure: " << ec.message() << std::endl;
        return false;
    }

    std::cout << "[UdpReceiver] Socket initialized successfully." << std::endl;
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

    std::cout << "[UdpReceiver] Socket" << name_ << ":" << port() << "closed." << std::endl;

    // Perparar por si hay reinicio
    clearQueue();
}

short UdpReceiver::port() const {
    if (socket_.is_open()) {
        return socket_.local_endpoint().port();
    } else {
        return -1; // Indica que el socket no está abierto
    }
}

std::string const& UdpReceiver::name() const {
    return name_;
}

bool UdpReceiver::isRunning() const {
    return socket_.is_open();
}

bool UdpReceiver::openSocket(short local_port, const std::string& local_ip) {

    // Variable para almacenar errores de asio
    asio::error_code ec;

    // Configuración del endpoint: IP/puerto
    asio::ip::udp::endpoint endpoint;
    
    // Si la ip local es vacía, se enlaza a todas las interfaces
    if (local_ip.empty()) {
        endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), local_port);
    }
    // Si se especifica una IP local, se enlaza a esa IP
    else {
        // Convertir la cadena a asio::ip::address
        asio::ip::address localIP = asio::ip::make_address(local_ip, ec);
        if (ec) {
            std::cerr << "[UdpReceiver] ERROR Invalid IP: " << local_ip << " - " << ec.message() << std::endl;
            return false;
        } 
        endpoint = asio::ip::udp::endpoint(localIP, local_port);
    }

    // Abrir el socket 
    socket_.open(endpoint.protocol(), ec);
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
    socket_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "[UdpReceiver] ERROR Failed binding socket: " << ec.message() << std::endl;
        return false;
    }

    /*else*/
    return true;
}

void UdpReceiver::start_receive() {
    if (!socket_.is_open()) return;

    auto self(shared_from_this());  // Mantiene el objeto vivo si se destruye antes
    
    socket_.async_receive_from(
        asio::buffer(recv_buffer_),
        remote_endpoint_,
        asio::bind_executor(strand_, [this, self](std::error_code ec, std::size_t bytes_recvd) {
            if (!ec) {
                // Pasamos a procesar el paquete
                handle_received_packet(bytes_recvd);
                // Pedimos el siguiente paquete SÓLO cuando terminamos con este
                start_receive();
            } else if (ec == asio::error::operation_aborted) {
                // Caso esperado al cerrar el socket o borrar el receptor
                std::cout << "[UdpReceiver] Socket " << name_ << " stopped successfuly." << std::endl;
            } else {
                // Continúa recibiendo a pesar del error (!abortar)
                std::cerr << "[UdpReceiver] "<< name_ << " ERROR Recepction error: " << ec.message() << std::endl;
                if (socket_.is_open()) {
                    start_receive();
                }
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

    if (queue_.empty()) 
        return {}; // En parada del socket devolvemos vector vacío

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
