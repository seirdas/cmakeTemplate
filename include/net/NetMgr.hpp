#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>               // std::string
#include <vector>               // Vectores
#include <thread>               // Hilos
#include <memory>               // unique_ptr


// Para no mezclar includes, declaración implícita
class UdpSocket;
struct NetPacket;


/**
  * @brief Gestiona varios sockets a partir de un único contexto de operaciones asíncronas (io_context)
  *  La gestión de los sockets de asio necesita un único io_context.
  * @class NetMgr
  * @brief Gestor de sockets UDP y del contexto de operaciones asíncronas (asio::io_context).
  * @details NetMgr centraliza la gestión de varios sockets UDP
  *   usando un único asio::io_context y un conjunto de hilos de trabajo. Permite
  *   añadir y eliminar sockets UDP.
  *  Comportamiento:
  *   * El io_context_ es único para todos los receptores y se mantiene vivo mediante
  *         work_guard_ para evitar que el contexto termine mientras haya hilos en espera.
  *   * addReceiver() crea y registra un UdpReceiver asociado a un puerto local.
  *   * removeReceiver() detiene y elimina el receptor asociado a un puerto.
  *   * start() crea hasta num_threads_ hilos que ejecutan io_context_.run().
  *   * stop() detiene todos los receptores y libera los hilos de trabajo.
  * @note num_threads_ determina el número máximo de hilos de trabajo (por defecto std::thread::hardware_concurrency()).
  * @note Los receptores se almacenan en receivers_ usando unique_ptr para gestión RAII.
  * @note El constructor es explicit para evitar conversiones implícitas en thread_count.
  * @see UdpSocket
  */
class NetMgr {

public:

// General ------------------------------------------------------------------------------
    
    /**
     * @brief Constructor.
     * @note explicit es para recibir literalmente un size_t en thread_count.
     * @param thread_count Número de threads máximo permitido por el sistema
     */
    explicit NetMgr(std::size_t const& thread_count = std::thread::hardware_concurrency());

    /**
     * @brief Destructor. Detiene todos los sockets.
     */
    ~NetMgr();

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    NetMgr(NetMgr const&) = delete;
    NetMgr& operator=(NetMgr const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    NetMgr(NetMgr&&) = delete;
    NetMgr& operator=(NetMgr&&) = delete;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicia un número de hilos con el contexto de operaciones asíncronas.
     * @details Toma por defecto el número máximo permitido por el sistema.
     * Esto no significa que no se puedan hacer más hilos, porque se quedan "idle" 
     * esperando datos en los sockets.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config = nullptr);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);

    /**
     * @brief Llama a init(), misma funcionalidad por compatibilidad.
     */    
    bool start();

    /**
     * @brief Detiene todos los sockets.
     */
    void stop();

    /**
     * @brief Detiene los sockets y cierra todos los recursos de la clase
     * @return @c true Si se ha cerrado correctamente, @c false en caso contrario
     */
    bool close();

    /**
     * @brief Devuelve si la red está activa (los sockets están activos)
     * @returns true si los sockets están activos, false en caso contrario
     */
    bool isRunning() const;
    

// Gestión de sockets -------------------------------------------------------------------

    /**
     * @brief Añade un socket.
     * @param name Nombre (arbitrario) asignado al socket
     * @param local_port Puerto local del socket
     * @param local_ip Ip local donde se asigna el socket.
     *  Si no se provee IP o está vacío, se asigna en todas las IP locales disponibles
     * @param rcv_packet_size Tamaño de paquete de recepción esperado.
     *  Si el paquete recibido no coincide con este tamaño, se rechazará.
     *  Si es =0 o no se especifica, se aceptarán todos los paquetes de cuelquier tamaño.
     * @return true si se ha registrado el socket correctamente, false en caso contrario.
     */
    bool addUdpSocket(
        std::string         name,
        unsigned short      local_port, 
        const std::string&  local_ip = "", 
        unsigned int        rcv_packet_size = 0
    );

    /**
     * @brief Detener y desvincular un socket activo por nombre
     * @param name Nombre del socket UDP gestionado
     * @return true si se ha eliminado correctamente, false en caso contrario.
     */
    bool removeUdpSocket(std::string const& name);

    /**
     * @brief Detener y desvincular un socket activo por puerto
     * @param port Puerto del socket UDP gestionado
     * @return true si se ha eliminado correctamente, false en caso contrario.
     */
    bool removeUdpSocket(unsigned int port);

    /**
     * @brief Devuelve el puntero a un socket UDP gestionado, buscado por nombre.
     * @details Pensado para consultar datos de un socket (nombre, puerto, último
     *  paquete...) desde código externo (GUI/CLI).
     * @param name Nombre del socket buscado
     * @return Puntero al UdpSocket si existe, @c nullptr en caso contrario.
     * @warning El puntero solo es válido mientras el socket siga registrado en NetMgr
     *  (no sobrevive a un removeUdpSocket() ni a la destrucción de NetMgr).
     */
    UdpSocket* getUdpSocket(std::string const& name) const;

    /**
     * @brief Devuelve los punteros a todos los sockets UDP gestionados actualmente.
     * @details Pensado para pintar el estado de los sockets en una UI (GUI/CLI).
     * @return Vector de UdpSocket*, en el mismo orden en que fueron añadidos.
     * @warning Los punteros solo son válidos mientras los sockets sigan registrados en NetMgr.
     */
    std::vector<UdpSocket*> getUdpSockets() const;

    /**
     * @brief Comprueba si un socket se está gestionando por nombre
     * @param name Nombre del socket
     * @return true si está en el vector de sockets gestionados, false en caso contrario.
     */
    bool socketExists(std::string const& socketname) const;

    /**
     * @brief Comprueba si un socket se está gestionando por puerto
     * @param port Puerto del socket
     * @return true si está en el vector de sockets gestionados, false en caso contrario.
     */
    bool socketExists(unsigned short port) const;

    /**
     * @brief Devuelve el tiempo que ha transcurrido desde que 
     *  ha recibido el último paquete de un socket UDP específico
     * @param name Nombre del socket del que obtener el tiempo
     * @return Tiempo transcurrido en ms
     */
    unsigned long long getLastPacketMs(std::string const& name) const;

    /**
     * @brief Devuelve el tiempo que ha transcurrido desde que 
     *  ha recibido el último paquete de un socket UDP específico
     * @param port Puerto del socket del que obtener el tiempo
     * @return Tiempo transcurrido en ms
     */
    unsigned long long getLastPacketMs(unsigned short port) const;


// Envío --------------------------------------------------------------------------------

    /**
     * @brief Manda datos por un socket creado por nombre
     * @param socketname Nombre del socket por el que se van a mandar los datos
     * @param data vector de bytes de datos a mandar
     * @param dest_ip IP destino
     * @param dest_port Puerto destino
     */
    bool sendData(
        std::string              socketname, 
        const std::vector<char>& data,
        const std::string&       dest_ip,
        unsigned short           dest_port
    );

    /**
     * @brief Manda datos por un socket creado por puerto
     * @param local_port Puerto del socket por el que se van a mandar los datos
     * @param data vector de bytes de datos a mandar
     * @param dest_ip IP destino
     * @param dest_port Puerto destino
     */
    bool sendData(
        unsigned short           local_port, 
        const std::vector<char>& data,
        const std::string&       dest_ip,
        unsigned short           dest_port
    );

// Recepción ----------------------------------------------------------------------------

    /**
     * @brief Número de paquetes de datos recibidos desde los sockets UDP en cola centralizada
     * @return Número de paquetes
     */
    size_t numUdpRcvElements();

    
private:

// Dispatcher ----------------------------------------------------------------------------

    /**
     * @brief Hilo gestor de paquetes online
     * @note Solo se ejecuta cuando la aplicación está en online, de lo contrario se queda parado.
     */
    void t_dispatcher();

    
// Datos de los sockets guardados -------------------------------------------------------

    /**
     * @brief Obtener el ID/index del socket por puerto.
     * @details Bloquea con lock el vector UdpSockets
     * @param port Puerto local del socket abierto asignado en su creación
     * @return Índice del vector udpSockets
     */
    int get_socket_index(short port) const;

    /**
     * @brief Obtener el ID/index del socket por nombre
     * @details Bloquea con lock el vector UdpSockets
     * @param name Nombre asignado al socket en su creación
     * @return Índice del vector udpSockets
     */
    int get_socket_index(std::string const& name) const;


// Operaciones privadas con sockets -----------------------------------------------------

    /**
     * @brief Detener y desvincular un socket activo por índice. 
     *  Supone que se ha adquirido lock de mutex
     * @details Es llamado por @c RemoveUdpSocket público
     * @param index Índice del vector udpsockets del socket a borrar
     * @return @c true Si se ha borrado correctamente
     */
    bool remove_udp_socket_locked(int index);


/************ Variables ****************************************************************/

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;                                            ///< Declaración de estructura PIMPL para no depender de la librería en el header
    std::unique_ptr<Impl>       pimpl_;                     ///< Miembros dependientes de la librería externa

// Inicialización
    bool                        initialized_;               ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>           running_;                   ///< flag de aplicación corriendo (para hilos)

// Contexto de operaciones asíncronas
    std::atomic<bool>           io_running_;                ///< Flag para saber si la red (io_context) está en funcionamiento
    std::vector<std::thread>    threads_;                   ///< Hilos procesando operaciones asíncronas.
    std::size_t                 num_threads_;               ///< Numero máximo de hilos gestionando operaciones asíncronas.

// Sockets
    mutable std::mutex          udp_sockets_mtx_;           ///< Mutex para gestion del vector de sockets
    std::atomic<bool>           sockets_running_;           ///< Flag para saber si la red (io_context) está en funcionamiento

// Cola global de datos de socket
    /* std::queue<NetPacket>    udp_rcv_data_; */           // en PIMPL
    mutable std::mutex          udp_rcv_data_mtx_;          ///< Mutex para cola de datos de sockets
    std::condition_variable     dispatcher_cv_;             ///< Condition variable para cola de datos de sockets
    const std::size_t           MAX_QUEUE_ELEMENTS_ = 100;  ///< Número máximo de elementos en la cola
    std::thread                 dispatcher_thread_;         ///< Hilo consumidor de paquetes de red

};
