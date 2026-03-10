#pragma once

#include <asio.hpp>             // asio external lib
#include <string>               // std::string
#include <vector>               // Vectores
#include <thread>               // Hilos
#include <memory>               // unique_ptr

// Para no mezclar includes, declaración implícita
class UdpReceiver;

/**
  * @brief Gestiona varios sockets a partir de un único contexto de operaciones asíncronas (io_context)
  *  La gestión de los sockets de asio necesita un único io_context.
  * @class NetMgr
  * @brief Gestor de sockets UDP y del contexto de operaciones asíncronas (asio::io_context).
  * @details NetMgr centraliza la gestión de varios receptores UDP (UdpReceiver)
  *   usando un único asio::io_context y un conjunto de hilos de trabajo. Permite
  *   añadir y eliminar receptores, así como iniciar y detener el grupo de hilos
  *   que procesan las operaciones asíncronas.
  *  Comportamiento:
  *      El io_context_ es único para todos los receptores y se mantiene vivo mediante
  *   work_guard_ para evitar que el contexto termine mientras haya hilos en espera.
  *      addReceiver() crea y registra un UdpReceiver asociado a un puerto local.
  *      removeReceiver() detiene y elimina el receptor asociado a un puerto.
  *      start() crea hasta thread_count_ hilos que ejecutan io_context_.run().
  *      stop() detiene todos los receptores y libera los hilos de trabajo.
  *  Uso típico:
  *      Instanciar NetMgr.
  *      Llamar addReceiver() según sea necesario.
  *      Llamar start() para iniciar el procesamiento en hilos.
  *      Cuando se termine, llamar stop() o destruir la instancia para limpiar recursos.
  * @note thread_count_ determina el número máximo de hilos de trabajo (por defecto std::thread::hardware_concurrency()).
  * @note Los receptores se almacenan en receivers_ usando unique_ptr para gestión RAII.
  * @note El constructor es explicit para evitar conversiones implícitas en thread_count.
  * @see UdpReceiver
  * @author
  * @date March 2, 2026 
  */
class NetMgr {

// General ------------------------------------------------------------------------------

public:

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

    /**
     * @brief Imprime por cout los sockets creados.
     */
    void printReceivers();

    /**
     * @brief Añade un socket.
     */
    bool addReceiver(
        std::string         name,
        short               local_port, 
        const std::string&  local_ip = "", 
        unsigned int        rcv_packet_size = 0
    );

    /**
     * @brief Detener y desvincular un socket activo por index.
     * @param index Índice de socket (en el orden en el que se crearon)
     */
    bool removeReceiver(unsigned int index);

    /**
     * @brief Obtener el ID/index del socket por puerto.
     */
    int getSocketIndex(short port) const;

    /**
     * @brief Obtener el ID/index del socket por nombre
     */
    int getSocketIndex(std::string const& name) const;

    /**
     * @brief Obtiene datos de la cola de datos del socket, identificado por nombre
     */
    std::vector<char> getDataFromSocket(unsigned int index);

    /**
     * @brief Inicia un número de hilos con el contexto de operaciones asíncronas.
     * @details Toma por defecto el número máximo permitido por el sistema.
     * Esto no significa que no se puedan hacer más hilos, porque se quedan "idle" 
     * esperando datos en los sockets.
     */
    void start();

    /**
     * @brief Detiene todos los sockets.
     */
    void stop();

    /**
     * @brief Devuelve si la red está activa (el contexto i/o está corriendo)
     */
    bool isRunning() const;

private:

    /************ Variables ********************************************************/

    // Contexto de operaciones asíncronas
    asio::io_context    io_context_;        // UN ÚNICO contexto de operaciones asíncronas para todo.
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_; // RAII para mantener vivo el io

    // Sockets
    std::vector<std::shared_ptr<UdpReceiver>> receivers_;   // Lista de receptores UDP registrados

    // Hilos de trabajo
    std::vector<std::thread>    threads_;           // Hilos procesando operaciones asíncronas.
    std::size_t                 thread_count_;      // Numero máximo de hilos gestionando operaciones asíncronas.
};
