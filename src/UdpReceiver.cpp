#include "UdpReceiver.hpp"
#include <iostream>
#include <atomic>

#define MAX_UDP_PACKET_SIZE 65536 // Tamaño máximo de un paquete UDP (64 KB)

// General ------------------------------------------------------------------------------

UdpReceiver::UdpReceiver(asio::io_context& io)
    : socket_(io), rcv_packet_size_(0)
{
    
}

UdpReceiver::~UdpReceiver() {
    stop();
}

bool UdpReceiver::init(short local_port, const std::string& local_ip, unsigned int rcv_packet_size) {
    
    // Reconstruir si ya existía 
    if (isRunning()) 
        stop();

    // Check puerto válido
    if (local_port==0) {
        std::cerr << "Port 0 is invalid." << std::endl;
        return false;
    }

    // Check tamaño de paquete válido
    if (rcv_packet_size > MAX_UDP_PACKET_SIZE) {
        std::cerr << "Packet size exceeds maximum UDP packet size." << std::endl;
        return false;
    }

    // Abrir (bind) el socket
    if (openSocket(local_port, local_ip) && socket_.is_open()) {
        std::cout << "Registering callback function..." << std::endl;
        start_receive();
    }
    else {
        std::cerr << "Socket is not open after bind." << std::endl;
        asio::error_code ec;
        socket_.close(ec);
        if (ec) std::cerr << "Error closing socket after failure: " << ec.message() << std::endl;
        return false;
    }

    // Redimensionar el buffer de recepción al tamaño esperado de los paquetes
    this->rcv_packet_size_ = rcv_packet_size;
    recv_buffer_.resize(rcv_packet_size_ > 0 ? rcv_packet_size_ : MAX_UDP_PACKET_SIZE);

    return true;
}

void UdpReceiver::stop() {
    asio::error_code ec;
    
    // Cerrar el socket si está abierto
    if (socket_.is_open()) {
        socket_.cancel(ec); // Cancelar cualquier operación pendiente
        socket_.close(ec);
    }
    if (ec) std::cerr << "Error cerrando socket: " << ec.message() << std::endl;

    // Despertar cualquier hilo que esté esperando en la cola de datos
    condition_.notify_all(); 

    // Perparar por si hay reinicio
    recv_buffer_.clear();
    clearQueue();
}

short UdpReceiver::port() const {
    if (socket_.is_open()) {
        return socket_.local_endpoint().port();
    } else {
        return -1; // Indica que el socket no está abierto
    }
}

bool UdpReceiver::isRunning() const {
    return socket_.is_open();
}

bool UdpReceiver::openSocket(short local_port, const std::string& local_ip){

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
            std::cerr << "Invalid IP: " << local_ip << " - " << ec.message() << std::endl;
            return false;
        } 
        endpoint = asio::ip::udp::endpoint(localIP, local_port);
    }

    // Abrir el socket 
    socket_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Error opening socket: " << ec.message() << std::endl;
        return false;
    }
    
    // Confirgurar el socket para permitir reutilizar la dirección local 
    socket_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Error setting socket option: " << ec.message() << std::endl;
        return false;
    }

    // Enlazar el socket al endpoint local
    socket_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Error binding socket: " << ec.message() << std::endl;
        return false;
    }

    /*else*/
    return true;
}

void UdpReceiver::start_receive() {
    
    // Cuando llegue UN paquete, se ejecutará la lambda (handle_received_packet).
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd) {
            // Esto se ejecuta cada vez que llega un paquete.
            handle_received_packet(ec, bytes_recvd);

            // Si sigue en ejecución, volver a activar el callback
            if (socket_.is_open())
                start_receive();
        }
    );

}

void UdpReceiver::handle_received_packet(std::error_code ec, std::size_t bytes_recvd) {
    // Si el programa se está cerrando o fue cancelado, salimos
    if (ec == asio::error::operation_aborted){
        std::cerr << "Receive operation aborted, stopping receiver." << std::endl;
        return;
    }

    // Si hubo un error en la recepción, lo reportamos y salimos
    if (ec) {
        std::cerr << "Receive error: " << ec.message() << std::endl;
        return;
    }

    // Si se ha recibido un paquete vacío, lo reportamos pero seguimos esperando
    if(bytes_recvd == 0) {
        std::cerr << "Received empty packet from " << remote_endpoint_ << std::endl;
        return;
    }

    // Si se ha especificado un tamaño esperado de paquete y el tamaño recibido es diferente, lo reportamos pero seguimos esperando
    if (rcv_packet_size_!=0 && bytes_recvd!=rcv_packet_size_){
        std::cerr << "Received packet size different from expected (" << bytes_recvd << " bytes, expected " << rcv_packet_size_ << " bytes) from " << remote_endpoint_ << std::endl;
        return;
    }

    // Si el paquete ya no entra en la cola, lo descartamos
    if (isQueueFull()){
        std::cerr << "Reception queue overloaded with " << getQueueSize() << " elements" << std::endl;
        return;
    }

    // Añade el paquete recibido a la cola listo para gestionar
    std::vector<char> data(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd);
    savePacket(std::move( data ) ); 
}



// Gestión de cola de datos ------------------------------------------------------------

void UdpReceiver::savePacket(std::vector<char> data) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(data));
    condition_.notify_one(); // Avisa que hay datos
}

std::vector<char> UdpReceiver::getFirstPacket() {
    std::unique_lock<std::mutex> lock(mutex_);
    // El hilo se duerme aquí hasta que la cola no esté vacía
    condition_.wait(lock, 
        [this] { 
            return !queue_.empty() || !socket_.is_open(); 
        }
    );

    if (queue_.empty()) 
        return {}; // Si se ha detenido el receptor y la cola está vacía, devolvemos un vector vacío

    std::vector<char> data = std::move(queue_.front());
    queue_.pop();
    return data;
}

bool UdpReceiver::isQueueEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

bool UdpReceiver::isQueueFull() const{
    std::lock_guard<std::mutex> lock(mutex_);
    return (queue_.size()>=MAX_QUEUE_ELEMENTS);
}

unsigned short UdpReceiver::getQueueSize() const{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();;
}

void UdpReceiver::clearQueue(){
    std::lock_guard<std::mutex> lock(mutex_);
    queue_ = std::queue<std::vector<char>>{};
}