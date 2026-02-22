#include "UdpReceiver.hpp"
#include <iostream>
#include <stdexcept>    // std::runtime_error
#include <atomic>

#define MAX_UDP_PACKET_SIZE 65536 // Tamaño máximo de un paquete UDP (64 KB)

// General ------------------------------------------------------------------------------

UdpReceiver::UdpReceiver() :
    socket_(io_context_), 
    rcv_packet_size_(0), 
    is_running_(false)
{
    
}

UdpReceiver::~UdpReceiver() {
    stop();
}

bool UdpReceiver::start(short LocalPort, const std::string& ipLocal, unsigned int rcv_packet_size) {
    
    // Check si estamos en ejecución 
    if (is_running_) 
        return true;
    
    // Check puerto válido
    if (LocalPort==0) {
        is_running_ = false;
        std::cerr << "Port 0 is invalid." << std::endl;
        return false;
    }

    // Check tamaño de paquete válido
    if (rcv_packet_size > MAX_UDP_PACKET_SIZE) {
        is_running_ = false;
        std::cerr << "Packet size exceeds maximum UDP packet size." << std::endl;
        return false;
    }

    // Variable para almacenar errores de asio
    asio::error_code ec;
    
    // Guardamos el tamaño esperado de los paquetes para validar en la recepción
    this->rcv_packet_size_ = rcv_packet_size;
        
    // Activamos el flag de ejecución antes de iniciar el proceso de recepción
    is_running_ = true;

    // Configuración del endpoint: IP/puerto
    asio::ip::udp::endpoint endpoint;
    
    // Si la ip local es vacía, se enlaza a todas las interfaces
    if (ipLocal.empty()) {
        endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), LocalPort);
    }
    // Si se especifica una IP local, se enlaza a esa IP
    else {
        // Convertir la cadena a asio::ip::address
        asio::ip::address localIP = asio::ip::make_address(ipLocal, ec);
        if (ec) {
            std::cerr << "Invalid IP: " << ipLocal << " - " << ec.message() << std::endl;
            return false;
        } 
        
        endpoint = asio::ip::udp::endpoint(localIP, LocalPort);
    }

    
    // Abrir el socket 
    socket_.open(endpoint.protocol(), ec);
    if (ec) std::cerr << "Error opening socket: " << ec.message() << std::endl;
    
    // Confirgurar el socket para permitir reutilizar la dirección local 
    socket_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) std::cerr << "Error setting socket option: " << ec.message() << std::endl;
    
    // Enlazar el socket al endpoint local
    socket_.bind(endpoint, ec);
    if (ec) std::cerr << "Error binding socket: " << ec.message() << std::endl;

    // Si hubo algún error crítico, cerramos el socket y salimos
    if (ec) {
        socket_.close(ec);
        if (ec) std::cerr << "Error closing socket after failure: " << ec.message() << std::endl;
        is_running_ = false;
        return false;
    }

    // Preparar el io_context por si hay reinicio
    io_context_.restart();   

    //  Registrar que quieres recibir datos.
    if (socket_.is_open())
        start_receive();
    else {
        is_running_ = false;
        std::cerr << "Socket is not open after bind." << std::endl;
        return false;
    }

    // Iniciar el hilo de trabajo para procesar las operaciones asíncronas
    worker_thread_ = std::thread([this]() { 
        // El hilo se queda ejecutando el io_context, procesando eventos del async_receive_from
        io_context_.run(); 
    });

    return true;
}

void UdpReceiver::stop() {
    is_running_ = false;
    asio::error_code ec;
    
    // Cerrar el socket si está abierto
    if (socket_.is_open()) {
        socket_.cancel(ec); // Cancelar cualquier operación pendiente
        socket_.close(ec);
    }
    if (ec) std::cerr << "Error cerrando socket: " << ec.message() << std::endl;

    // Detener el io_context
    io_context_.stop();

    // Despertar cualquier hilo que esté esperando en la cola de datos
    condition_.notify_all(); 

    // Esperar a que el hilo termine y cerrar el socket
    if (worker_thread_.joinable()) 
        worker_thread_.join();

    // Perparar por si hay reinicio
    io_context_.restart();
    recv_buffer_.clear();
}

short UdpReceiver::getLocalPort() const {
    if (socket_.is_open()) {
        return socket_.local_endpoint().port();
    } else {
        return -1; // Indica que el socket no está abierto
    }
}

bool UdpReceiver::isRunning() const {
    return is_running_;
}

void UdpReceiver::start_receive() {

    //Cuando llegan datos:
    //  El SO notifica
    //  io_context despierta
    //  Ejecuta tu lambda

    // Redimensionar el buffer de recepción al tamaño esperado de los paquetes
    try {
        recv_buffer_.resize(rcv_packet_size_ > 0 ? rcv_packet_size_ : MAX_UDP_PACKET_SIZE);
    
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
        std::cout << "async_receive_from called successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in start_receive: " << e.what() << std::endl;
    }

}

void UdpReceiver::handle_received_packet(std::error_code ec, std::size_t bytes_recvd) {
    // Si el programa se está cerrando o fue cancelado, salimos
    if (!is_running_ && ec == asio::error::operation_aborted){
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