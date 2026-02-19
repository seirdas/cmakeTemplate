#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

/**
 * @brief Gestiona la recepción de datos UDP de forma asíncrona. Permite configurar la IP local, el puerto y el tamaño máximo de los paquetes.
 *          Los datos recibidos se almacenan en una cola compartida, accesible mediante métodos de push/pop. 
 *          El receptor se ejecuta en un hilo separado para no bloquear el hilo principal.
 */
class UdpReceiver {

public:

    /**
     * @brief Constructor de UdpReceiver. Configura el socket UDP para recibir datos en la IP y puerto especificados.
     *        Si la IP local es vacía, se enlaza a todas las interfaces disponibles
     * @param ipLocal - Dirección IP local a la que se desea enlazar el socket. Si es vacía, se enlaza a todas las interfaces disponibles.
     * @param port - Puerto en el que se desea recibir los datos UDP.
     * @param packet_size - Tamaño máximo de los paquetes UDP que se esperan recibir. Elimina el paquete si es diferente a este tamaño. Si es 0, se aceptan paquetes de cualquier tamaño.
     */
    UdpReceiver(const std::string& ipLocal = "", short port, unsigned int _packet_size = 0) 
        : socket_(io_context_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
        packet_size(_packet_size)
        {
            is_running_ = false;
            asio::ip::udp::endpoint endpoint;
            
            // Si la ip local es vacía, se enlaza a todas las interfaces
            if (ipLocal.empty())
                endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), port);
            // Si se proporciona una IP local, se enlaza a esa IP concreta
            else {
                // Convertir la cadena a asio::ip::address
                asio::ip::address address = asio::ip::make_address(ipLocal, ec);
                if (ec) throw std::runtime_error("IP no válida: " + ipLocal + " - " + ec.message());
    
                asio::ip::udp::endpoint endpoint(address, port);          // 2️⃣ Crear el endpoint con la IP y el puerto deseado
    
                socket_.open(asio::ip::udp::v4(), ec);                    // 3️⃣ Abrir el socket (IPv4)
                if (ec) throw std::runtime_error("Error opening socket: " + ec.message());
    
                socket_.bind(endpoint, ec);                      // 4️⃣ Enlazar (bind) al endpoint concreto
                if (ec) throw std::runtime_error("Error binding socket: " + ec.message());
            }
            socket_ = asio::ip::udp::socket(io_context_, endpoint);
        }


    //Primero:
    //  Registras que quieres recibir datos.
    //  Arrancas el hilo que ejecuta el loop.
    //Cuando llegan datos:
    //  El SO notifica
    //  io_context despierta
    //  Ejecuta tu lambda

    void start() {
        if (is_running_) return;
        is_running_ = true;
        start_receive();

        // Iniciar el hilo de trabajo para procesar las operaciones asíncronas
        worker_thread_ = std::thread([this]() { 
            // El hilo se queda ejecutando el io_context, procesando eventos del async_receive_from
            io_context_.run(); 
        });
    }

    void stop() {
        is_running_ = false;
        io_context_.stop();
        if (worker_thread_.joinable()) worker_thread_.join();
    }

private:
    void start_receive() {

        // Se espera aquí a que llegue un paquete UDP. Cuando llegue, se ejecutará la lambda.
        socket_.async_receive_from(
            asio::buffer(recv_buffer_), remote_endpoint_,
            [this](std::error_code ec, std::size_t bytes_recvd) {
                // Esto se ejecuta cada vez que llega un paquete.
                handle_receive(ec, bytes_recvd);
                start_receive(); // Volver a esperar el siguiente paquete
            }
        );
    }

    void handle_receive(std::error_code ec, std::size_t bytes_recvd) {
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
        queue_.push(std::move( std::vector(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd)) );
            
    }

    // Variables miembro 

    asio::io_context io_context_;               // Contexto de E/S para operaciones asíncronas
    asio::ip::udp::socket socket_;              // Socket UDP para recibir datos
    asio::ip::udp::endpoint remote_endpoint_;   // Endpoint remoto desde el que se reciben los datos
    std::vector<char> recv_buffer_;             // Buffer para almacenar los datos recibidos
    unsigned int packet_size;                   // Tamaño esperado de los paquetes UDP (0 para aceptar cualquier tamaño)
    std::thread worker_thread_;                 // Hilo para ejecutar el io_context y procesar eventos asíncronos
    std::atomic<bool> is_running_;              // Flag para controlar el estado de ejecución 
    asio::error_code ec;                        // Variable para almacenar errores de asio


// Gestión de cola de datos ------------------------------------------------------------
public:
    std::vector<char> getNextPacket() {
        return pop();
    }


private:
    void push(std::vector<char> data) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(data));
        condition_.notify_one(); // Avisa al main que hay datos
    }

    std::vector<char> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        // El hilo se duerme aquí hasta que la cola no esté vacía
        condition_.wait(lock, [this] { return !queue_.empty(); });

        std::vector<char> data = std::move(queue_.front());
        queue_.pop();
        return data;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    

    std::queue<std::vector<char>> queue_;   // Cola de datos recibidos
    mutable std::mutex mutex_;              // Mutex para proteger el acceso a la cola
    std::condition_variable condition_;     // Condición para notificar al main que hay datos nuevos

};