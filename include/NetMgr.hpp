#pragma once

#include <asio.hpp>             // asio external lib
#include <string>               // std::string
#include <vector>               // Vectores
#include <thread>               // Hilos
#include "UdpReceiver.hpp"

/**
 * @brief Gestiona varios sockets a partir de un único contexto de operaciones asíncronas (io_context)
 * La gestión de los sockets de asio necesita un único io_context.
 */
class NetMgr {
public:

    /**
     * @brief Constructor.
     * @note explicit es para recibir literalmente un size_t en thread_count.
     * @param thread_count Número de threads máximo permitido por el sistema
     */
    explicit NetMgr(std::size_t thread_count = std::thread::hardware_concurrency());

    /**
     * @brief Destructor. Detiene todos los sockets.
     */
    ~NetMgr();

    /**
     * @brief Añade un socket.
     */
    bool addReceiver(
        short               local_port, 
        const std::string&  local_ip = "", 
        unsigned int        rcv_packet_size = 0
    );

    /**
     * @brief Detener y desvincular un socket activo.
     */
    bool removeReceiver(short port);

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

private:

    /************ Variables ********************************************************/

    // Contexto de operaciones asíncronas
    asio::io_context    io_context_;        // UN ÚNICO contexto de operaciones asíncronas para todo.
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_; // RAII para mantener vivo el io

    // Sockets e hilos de trabajo
    std::unordered_map<short, std::unique_ptr<UdpReceiver>> receivers_;     // Mapa de sockets abiertos.
    std::vector<std::thread>                                threads_;       // Hilos procesando operaciones asíncronas.
    std::size_t                                             thread_count_;  // Numero máximo de hilos gestionando operaciones asíncronas.
};