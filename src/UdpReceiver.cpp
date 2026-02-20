#include "UdpReceiver.hpp"
#include <iostream>
#include <stdexcept>    // std::runtime_error
#include <atomic>

#define MAX_UDP_PACKET_SIZE 65536 // Tamaño máximo de un paquete UDP (64 KB)

// General ------------------------------------------------------------------------------

UdpReceiver::UdpReceiver(short _LocalPort, const std::string& _ipLocal, unsigned int _rcv_packet_size) :
    socket_(io_context_), 
    packet_size(_rcv_packet_size), 
    is_running_(false)
{
    if (_LocalPort==0) 
        throw std::runtime_error("Port 0 is invalid.");

    asio::ip::udp::endpoint endpoint;
    
    // Si la ip local es vacía, se enlaza a todas las interfaces
    if (_ipLocal.empty())
    endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), _LocalPort);
    
    // Si se especifica una IP local, se enlaza a esa IP
    else {
        // Convertir la cadena a asio::ip::address
        asio::ip::address localIP = asio::ip::make_address(_ipLocal, ec);
        if (ec) 
        throw std::runtime_error("IP no válida: " + _ipLocal + " - " + ec.message());
        
        endpoint = asio::ip::udp::endpoint(localIP, _LocalPort);
    }
    
    try {
        socket_.open(endpoint.protocol(), ec);
        if (ec) throw std::system_error(ec);
        
        socket_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) throw std::system_error(ec);
        
        socket_.bind(endpoint, ec);
        if (ec) throw std::system_error(ec);
        
    }
    catch (const std::system_error& e) {
        if (socket_.is_open()) socket_.close(ec);
        if (ec) std::cerr << "Error cerrando socket tras fallo: " << ec.message() << std::endl;
        
        throw std::runtime_error("Error creating socket: " + std::string(e.what()));
    }

    // Redimensionar el buffer de recepción al tamaño esperado de los paquetes
    recv_buffer_.resize(packet_size > 0 ? packet_size : MAX_UDP_PACKET_SIZE);
}

UdpReceiver::~UdpReceiver() {
    stop();
}

void UdpReceiver::start() {
    if (is_running_) return;
    is_running_ = true;
    start_receive();

    // Iniciar el hilo de trabajo para procesar las operaciones asíncronas
    worker_thread_ = std::thread([this]() { 
        // El hilo se queda ejecutando el io_context, procesando eventos del async_receive_from
        io_context_.run(); 
    });
}

void UdpReceiver::stop() {
    is_running_ = false;

    // Detener el io_context
    io_context_.stop();
    
    // Cerrar el socket si está abierto
    if (socket_.is_open()) socket_.close(ec);

    // Despertar cualquier hilo que esté esperando en la cola de datos
    condition_.notify_all(); 

    // Esperar a que el hilo termine y cerrar el socket
    if (worker_thread_.joinable()) worker_thread_.join();
}

void UdpReceiver::start_receive() {

    // Se espera aquí a que llegue un paquete UDP. Cuando llegue, se ejecutará la lambda.
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd) {
            // Esto se ejecuta cada vez que llega un paquete.
            handle_received_packet(ec, bytes_recvd);

            // Si sigue en ejecución, sigue procesando
            if (is_running_.load(std::memory_order_acquire))
                start_receive();
        }
    );

}

void UdpReceiver::handle_received_packet(std::error_code ec, std::size_t bytes_recvd) {
    // Si el programa se está cerrando o fue cancelado, salimos
    if (!is_running_ && ec == asio::error::operation_aborted){
        std::cerr << "Receive operation aborted, stopping receiver." << std::endl;
        return;
    }

    if (ec) {
        std::cerr << "Receive error: " << ec.message() << std::endl;
        return;
    }

    if(bytes_recvd == 0) {
        std::cerr << "Received empty packet from " << remote_endpoint_ << std::endl;
        return;
    }

    if (packet_size!=0 && bytes_recvd!=packet_size){
        std::cerr << "Received packet size different from expected (" << bytes_recvd << " bytes, expected " << packet_size << " bytes) from " << remote_endpoint_ << std::endl;
    }

    // Añade el paquete recibido a la cola listo para gestionar
    std::vector data = std::vector(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd);
    savePacket(std::move( data ) );
        
}



// Gestión de cola de datos ------------------------------------------------------------

void UdpReceiver::savePacket(std::vector<char> data) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(data));
    condition_.notify_one(); // Avisa al main que hay datos
}

std::vector<char> UdpReceiver::getFirstPacket() {
    std::unique_lock<std::mutex> lock(mutex_);
    // El hilo se duerme aquí hasta que la cola no esté vacía
    condition_.wait(lock, 
        [this] { 
            return !queue_.empty() || !is_running_; 
        }
    );

    if (queue_.empty()) 
        return {}; // Si se ha detenido el receptor y la cola está vacía, devolvemos un vector vacío

    std::vector<char> data = std::move(queue_.front());
    queue_.pop();
    return data;
}

bool UdpReceiver::isPacketsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}