#pragma once

#include <asio.hpp>             // asio external lib
#include <queue>                // Colas
#include <mutex>                // Mutex (Cerrojos) para concurrencia
#include <condition_variable>   // Variable condicional para concurrencia
#include <vector>               // Vectores

/**
  * @class UdpSocket
  * @brief Receptor UDP asíncrono ejecutado por el io_context de NetMgr.
  * @details UdpSocket gestiona la recepción de paquetes UDP usando un
  * asio::io_context proporcionado externamente. 
  * Proporciona:
  *     Inicialización y enlace del socket a una IP/puerto locales.
  *     Recepción asíncrona de datos y almacenamiento en una cola protegida.
  *     Operaciones thread-safe para obtener paquetes (bloqueante), descartar
  *     duplicados y controlar el ciclo de vida del receptor.
  * Diseño y comportamiento:
  *     Requiere un único io_context (normalmente gestionado por NetMgr) para
  * funcionar correctamente.
  *     Ejecuta las operaciones de E/S en un hilo separado para no bloquear el
  * hilo principal de la aplicación.
  *     La cola de paquetes está protegida por mutex y condition_variable; hasta
  * MAX_QUEUE_ELEMENTS paquetes se mantienen en memoria.
  *     Si rcv_packet_size_ != 0, los paquetes de tamaño distinto se descartan.
  *     El flag ignore_dupe_ permite evitar almacenar paquetes idénticos al
  * último recibido.
  * Seguridad y notas de implementación:
  *     Las funciones públicas que acceden a la cola son seguras para concurrencia
  * salvo indicación contraria; los datos se transfieren por copia para
  * evitar condiciones de carrera.
  *     Evitar operaciones bloqueantes o de larga duración en callbacks de
  * E/S (handle_received_packet) para no retrasar la aceptación de nuevos
  * paquetes.
  *     El destructor detiene la recepción y cierra el socket para garantizar una
  * limpieza correcta.
  * @see NetMgr, asio::io_context
  */
class UdpSocket : public std::enable_shared_from_this<UdpSocket> {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor. 
     * @param io Referencia a contexto de operaciones asíncronas.
     */
    UdpSocket(std::string const& name, asio::io_context& io);

    /**
     * @brief Destructor. Detiene la recepción de datos y cierra el socket.
     */
    ~UdpSocket();

    
// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa el socket: Configura el socket UDP para recibir datos en la IP y puerto especificados.
     *  Si la IP local es vacía, se enlaza a todas las interfaces disponibles
     * @param io Contexto de operaciones asíncronas.
     * @param LocalPort Puerto en el que se desea recibir los datos UDP.
     * @param ipLocal Dirección IP local a la que se desea enlazar el socket. 
     *  Si es vacía, se enlaza a todas las interfaces disponibles.
     * @param rcv_packet_size Tamaño máximo de los paquetes UDP que se esperan recibir. 
     *  Elimina el paquete si es diferente a este tamaño. 
     *  Si es 0, se aceptan paquetes de cualquier tamaño.
     */
    bool init(unsigned short local_port, const std::string& local_ip = "", unsigned int rcv_packet_size = 0);

    /**
     * @brief Registra el callback de recepción de datos.
     * @note Usa las características de shared_from_this
     */
    void start();

    /**
     * @brief Si está en ejecución, detiene la recepción de datos UDP, cierra el socket y finaliza el hilo de trabajo.
     */
    void stop();

    /**
     * @brief Manda un paquete a un socket
     * @param data  Datos a mandar
     * @param dest_ip    IP destino
     * @param dest_port  Puerto destino
     * @return true si envío correcto, false en caso contrario
     */
    bool sendPacket(const std::vector<char>& data, const std::string& ip, unsigned short port);

// Datos de socket ----------------------------------------------------------------------

    /**
     * @brief Devuelve el puerto local al que está enlazado el socket.
     * @return El puerto local del socket UDP. Si el receptor no está en ejecución, devuelve -1.
     */
    short port() const;

    /**
     * @brief Devuelve el nombre dado al socket
     * @return Nombre del socket
     */
    std::string const& name() const;

    /**
     * @brief Devuelve si el receptor UDP está en ejecución
     * @return true si el receptor UDP está en ejecución, false en caso contrario.
     */
    bool isRunning() const;

    /**
     * @brief Devuelve si el socket está abierto
     * @return true si el socket está abierto, false en caso contrario.
     */
    bool isOpen() const;


// Gestión de la cola de datos recibidos ------------------------------------------------

    /**
     * @brief Devuelve el primer paquete recibido de la cola. Si la cola está vacía, espera hasta que llegue un nuevo paquete.
     */
    std::vector<char> getFirstPacket();
    
    /**
     * @brief Opción para rechazar el paquete de datos si es igual que el último recibido.
     * @param enable true para descartar duplicado, false en caso contrario.
     */
    void discardOnDupe(bool enable);

    /**
     * @brief Método público para limpiar la cola de datos.
     */
    void clearCache();

	/**
	 * @brief Devuelve si el socket tiene datos en su "buffer"
	 */
    bool hasData();

    /**
     * @brief Obtiene una referencia al strand que coordina las operaciones del receptor.
     * * El strand garantiza que todos los handlers (lectura, inicio, parada) se ejecuten
     * de forma secuencial, incluso si el io_context dispone de un pool de múltiples hilos.
     * * @note Utilice esta referencia para postear tareas externas que deban interactuar 
     * de forma segura con el socket o los buffers internos (ej. @ref stop o @ref start).
     * * @warning No capture esta referencia en lambdas que puedan sobrevivir al objeto UdpSocket;
     * asegúrese siempre de que la vida del objeto está garantizada (ej. mediante shared_from_this).
     * * @return Referencia al asio::strand asociado a este receptor.
     */
    asio::strand<asio::io_context::executor_type>& getStrand() { return strand_; }


private:
    
// Socket -------------------------------------------------------------------------------

    /**
     * @brief Crear endpoint local con IP+puerto en variable miembro local_endpoint
     * @return true si se ha creado correctamente, false en caso contrario
     */
    bool createLocalEndpoint(unsigned short local_port, const std::string& local_ip);

    /**
     * @brief Inicialización y linkado (bind) del socket según endpoint local miembro
     * @returns true si se ha creado el socket correctamente, false en caso contrario.
     */
    bool openSocket();

    /**
     * @brief Callback cuando recibe un paquete de datos
     */
    void start_receive();

    /**
     * @brief Guarda el paquete recibido en una cola de datos bajo unas condiciones.
     */
    void handle_received_packet(std::size_t bytes_recvd);


// Gestión de la cola de datos recibidos ------------------------------------------------

    /**
     * @brief Añade un paquete recibido a la cola de datos. 
     */
    void saveToQueue(std::vector<char> data);

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
    size_t getQueueSize() const;

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


/************ Variables ****************************************************************/
    using CStrand = asio::strand<asio::io_context::executor_type>;
    using CSocket = asio::basic_datagram_socket<asio::ip::udp, asio::io_context::executor_type>;

    // Configuración y estado del receptor UDP
    std::string                 name_;              // Nombre dado al socket para identificarlo
    CStrand                     strand_;            // Protección del buffer asíncrono de recepción
    CSocket                     socket_;            // Socket asio
    asio::ip::udp::endpoint     local_endpoint_;    // Endpoint local (ip+puerto local)
    asio::ip::udp::endpoint     remote_endpoint_;   // Endpoint remoto desde el que se reciben los datos
    std::vector<char>           recv_buffer_;       // Buffer para almacenar los datos recibidos
    unsigned int                rcv_packet_size_;   // Tamaño esperado de los paquetes UDP (0 para aceptar cualquier tamaño)

    bool                        initialized_;       // Socket inicializado
    bool                        running_;           // Socket corriendo

    // Cola de datos recibidos
    std::queue<std::vector<char>>   queue_;                     // Cola de datos recibidos
    mutable std::mutex              mutex_;                     // Mutex para proteger el acceso a la cola
    std::condition_variable         condition_;                 // Condición para notificar al main que hay datos nuevos
    const std::size_t               MAX_QUEUE_ELEMENTS = 20;    // Número máximo de elementos en la cola
    bool                            ignore_dupe_;               // Flag para eliminar duplicados
};