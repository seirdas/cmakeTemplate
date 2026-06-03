#pragma once

#include <mutex>
#include <string>               // std::string
#include <vector>               // Vectores
#include <thread>               // Hilos
#include <memory>               // unique_ptr
#include <queue>                // Cola de datos de los sockets

// Para no mezclar includes, declaración implícita
class UdpSocket;

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
  *   * start() crea hasta thread_count_ hilos que ejecutan io_context_.run().
  *   * stop() detiene todos los receptores y libera los hilos de trabajo.
  * @note thread_count_ determina el número máximo de hilos de trabajo (por defecto std::thread::hardware_concurrency()).
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
     * @brief Imprime por cout los sockets creados.
     */
    void printUdpSockets() const;

    /**
     * @brief Comprueba si un socket se está gestionando por nombre
     * @param name Nombre del socket
     * @return true si está en el vector de sockets gestionados, false en caso contrario.
     */
    bool socketExists(std::string const& socketname);

    /**
     * @brief Comprueba si un socket se está gestionando por puerto
     * @param port Puerto del socket
     * @return true si está en el vector de sockets gestionados, false en caso contrario.
     */
    bool socketExists(unsigned short port);


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicia un número de hilos con el contexto de operaciones asíncronas.
     * @details Toma por defecto el número máximo permitido por el sistema.
     * Esto no significa que no se puedan hacer más hilos, porque se quedan "idle" 
     * esperando datos en los sockets.
     * @returns true cuando termina
     */
    bool start();

    /**
     * @brief Detiene todos los sockets.
     */
    void stop();

    /**
     * @brief Devuelve si la red está activa (los sockets están activos)
     * @returns true si los sockets están activos, false en caso contrario
     */
    bool isRunning() const;
    

// Envío --------------------------------------------------------------------------------

    /**
     * @brief Manda datos por un socket
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
     * @brief Manda datos por un socket por puerto
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
     * @brief Obtiene datos de la cola de datos del socket, identificado por nombre
     * @warning BLOQUEANTE
     * @returns Datos recibidos del socket en formato std::vector<char>
     */
    std::vector<char> getDataFromSocket(std::string const& socketname);

    /**
     * @brief Obtiene datos de la cola de datos del socket, identificado por nombre
     * @warning BLOQUEANTE
     * @returns Datos recibidos del socket en formato std::vector<char>
     */
    std::vector<char> getDataFromSocket(unsigned short local_port);


private:

// Datos de los sockets guardados -------------------------------------------------------

    /**
     * @brief Obtener el ID/index del socket por puerto.
     * @details Bloquea con lock el vector UdpSockets
     * @param port Puerto local del socket abierto asignado en su creación
     * @return Índice del vector udpSockets
     */
    int getSocketIndex(short port) const;

    /**
     * @brief Obtener el ID/index del socket por nombre
     * @details Bloquea con lock el vector UdpSockets
     * @param name Nombre asignado al socket en su creación
     * @return Índice del vector udpSockets
     */
    int getSocketIndex(std::string const& name) const;


// Operaciones privadas con sockets -----------------------------------------------------

    /**
     * @brief Detener y desvincular un socket activo por índice
     * @details Es llamado por @c RemoveUdpSocket público
     * @param index Índice del vector udpsockets del socket a borrar
     * @return @c true Si se ha borrado correctamente
     */
    bool RemoveUdpSocketLocked(int index);


/************ Variables ****************************************************************/

    // Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>       pimpl_;             ///< Miembros dependientes de la librería externa

    // Contexto de operaciones asíncronas
    std::atomic<bool>           io_running_;        ///< Flag para saber si la red (io_context) está en funcionamiento
    
    // Sockets
    mutable std::mutex          udp_sockets_mtx_;   ///< Mutex para gestion del vector de sockets
    std::atomic<bool>           sockets_running_;   ///< Flag para saber si la red (io_context) está en funcionamiento

    // Cola global de datos de socket
    std::queue<NetPacket>       udp_rcv_data_;      ///< Cola de datos recibidos por los sockets UDP
    mutable std::mutex          udp_rcv_data_mtx_;  ///< Mutex para cola de datos de sockets
    mutable std::mutex          udp_rcv_data_cv_;   ///< Condition variable para cola de datos de sockets

    // Hilos de trabajo
    std::vector<std::thread>    threads_;           ///< Hilos procesando operaciones asíncronas.
    std::size_t                 thread_count_;      ///< Numero máximo de hilos gestionando operaciones asíncronas.
};
