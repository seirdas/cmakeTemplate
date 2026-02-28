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
 * @note NECESITA asio::io_context, preferiblemente desde una clase controladora (NetMgr) 
 */
class UdpReceiver {

public:

    // Socket ------------------------------------------------------------

    /**
     * @brief Constructor. 
     * @param io Referencia a contexto de operaciones asíncronas.
     */
    UdpReceiver(asio::io_context& io);

    /**
     * @brief Destructor. Detiene la recepción de datos y cierra el socket.
     */
    ~UdpReceiver();

    /**
     * @brief Inicializa el socket: Configura el socket UDP para recibir datos en la IP y puerto especificados.
     *        Si la IP local es vacía, se enlaza a todas las interfaces disponibles
     * @param io Contexto de operaciones asíncronas.
     * @param LocalPort Puerto en el que se desea recibir los datos UDP.
     * @param ipLocal Dirección IP local a la que se desea enlazar el socket. Si es vacía, se enlaza a todas las interfaces disponibles.
     * @param rcv_packet_size Tamaño máximo de los paquetes UDP que se esperan recibir. Elimina el paquete si es diferente a este tamaño. Si es 0, se aceptan paquetes de cualquier tamaño.
     */
    bool init(short local_port, const std::string& local_ip = "", unsigned int rcv_packet_size = 0);

    /**
     * @brief Si está en ejecución, detiene la recepción de datos UDP, cierra el socket y finaliza el hilo de trabajo.
     */
    void stop();

    /**
     * @brief Devuelve el puerto local al que está enlazado el socket.
     * @return El puerto local del socket UDP. Si el receptor no está en ejecución, devuelve -1.
     */
    short port() const;

    /**
     * @brief Devuelve si el receptor UDP está en ejecución
     * @return true si el receptor UDP está en ejecución, false en caso contrario.
     */
    bool isRunning() const;

    // Gestión de la cola de datos recibidos ------------------------------------------------------------

    /**
     * @brief Devuelve el primer paquete recibido de la cola. Si la cola está vacía, espera hasta que llegue un nuevo paquete.
     */
    std::vector<char> getFirstPacket();
    
    /**
     * @brief Opción para rechazar el paquete de datos si es igual que el último recibido.
     * @param enable true para descartar duplicado, false en caso contrario.
     */
    void discardOnDupe(bool enable);

private:
    
    // Socket ------------------------------------------------------------

    /**
     * @brief Inicialización y linkado (bind) del socket según IP y puerto
     * @returns true si se ha creado el socket correctamente, false en caso contrario.
     */
    bool openSocket(short local_port, const std::string& local_ip);

    /**
     * @brief Registra el callback de recepción de datos.
     */
    void start_receive();

    /**
     * @brief Guarda el paquete recibido en una cola de datos bajo unas condiciones.
     */
    void handle_received_packet(std::error_code ec, std::size_t bytes_recvd);


    // Gestión de la cola de datos recibidos ------------------------------------------------------------

    /**
     * @brief Añade un paquete recibido a la cola de datos. 
     */
    void savePacket(std::vector<char> data);

    /**
     * @brief Devuelve si la cola de datos recibidos está vacía.
     * @return true si la cola está vacía, false en caso contrario.
     */
    bool isQueueEmpty() const;

    /**
     * @brief Devuelve si la cola de datos recibidos está llena
     * @return true si la cola está llena, false en caso contrario.
     */
    bool isQueueFull() const;

    /**
     * @brief Devuelve el número de elementos en la cola.
     * @return Número de elementos de la cola.
     */
    unsigned short getQueueSize() const;

    /**
     * @brief Limpia la cola.
     * @details Crea una cola nueva en la misma variable.
     */
    void clearQueue();

    /**
     * @brief Compara un paquete de datos con el primer elemento que se va a obtener de la tabla
     * @return true si es igual, false en caso contrario.
     */
    bool compareLast(std::vector<char> const& data);


    /************ Variables ********************************************************/

    // Configuración y estado del receptor UDP
    asio::ip::udp::socket   socket_;            // Socket UDP para recibir datos
    asio::ip::udp::endpoint remote_endpoint_;   // Endpoint remoto desde el que se reciben los datos
    std::vector<char>       recv_buffer_;       // Buffer para almacenar los datos recibidos
    unsigned int            rcv_packet_size_;   // Tamaño esperado de los paquetes UDP (0 para aceptar cualquier tamaño)
    std::thread             worker_thread_;     // Hilo para ejecutar el io_context y procesar eventos asíncronos

    // Cola de datos recibidos
    std::queue<std::vector<char>> queue_;       // Cola de datos recibidos
    mutable std::mutex            mutex_;       // Mutex para proteger el acceso a la cola
    std::condition_variable       condition_;   // Condición para notificar al main que hay datos nuevos
    const std::size_t             MAX_QUEUE_ELEMENTS = 20;  // Número máximo de elementos en la cola
    bool                          ignore_dupe_; // Flag para eliminar el paquete si es el mismo que el anterior
};