#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>


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
     * @param LocalPort Puerto en el que se desea recibir los datos UDP.
     * @param ipLocal Dirección IP local a la que se desea enlazar el socket. Si es vacía, se enlaza a todas las interfaces disponibles.
     * @param packet_size Tamaño máximo de los paquetes UDP que se esperan recibir. Elimina el paquete si es diferente a este tamaño. Si es 0, se aceptan paquetes de cualquier tamaño.
     */
    UdpReceiver(short LocalPort = 0, const std::string& ipLocal = "", unsigned int _rcv_packet_size = 0);

    /**
     * @brief Destructor de UdpReceiver. Detiene la recepción de datos y cierra el socket.
     */
    ~UdpReceiver();

    //Primero:
    //  Registras que quieres recibir datos.
    //  Arrancas el hilo que ejecuta el loop.
    //Cuando llegan datos:
    //  El SO notifica
    //  io_context despierta
    //  Ejecuta tu lambda

    void start();

    void stop();

    bool isRunning() const { return is_running_; }

private:
    
    void start_receive();

    void handle_received_packet(std::error_code ec, std::size_t bytes_recvd);

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

    std::vector<char> getFirstPacket();


private:


    void savePacket(std::vector<char> data);

    std::vector<char> pop();

    bool isPacketsEmpty() const;


    std::queue<std::vector<char>> queue_;   // Cola de datos recibidos
    mutable std::mutex mutex_;              // Mutex para proteger el acceso a la cola
    std::condition_variable condition_;     // Condición para notificar al main que hay datos nuevos

};