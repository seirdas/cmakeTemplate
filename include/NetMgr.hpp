#pragma once

#include <asio.hpp>             // asio external lib
#include <vector>               // Vectores
#include <memory>               // Gestión de memoria
#include <thread>               // Hilos
#include <iostream>             // Entrada/Salida estándar
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
    explicit NetMgr(std::size_t thread_count = std::thread::hardware_concurrency())
        : work_guard_(asio::make_work_guard(io_context_)),
          thread_count_(thread_count == 0 ? 1 : thread_count)
    {

    }

    /**
     * @brief Destructor. Detiene todos los sockets.
     */
    ~NetMgr() {
        stop();
    }

    /**
     * @brief Añade un socket.
     */
    bool addReceiver(short local_port, const std::string& local_ip = "", unsigned int rcv_packet_size = 0) {

        // Evitar duplicados
        if (receivers_.find(local_port) != receivers_.end()) {
            std::cerr << "NetMgr: Socket already exists on port " << local_port << std::endl;
            return false;
        }

        // Crear receiver (aún no registrado)
        auto receiver = std::make_unique<UdpReceiver>(io_context_);

        // Intentar inicializar
        if (!receiver->init(local_port, local_ip, rcv_packet_size))
        {
            std::cerr << "NetMgr: Failed to initialize socket on port " << local_port << std::endl;
            return false;
        }

        // Insertar en el vector
        receivers_.emplace(local_port, std::move(receiver));
        std::cout << "NetMgr: Socket added on port " << local_port << "\n";

        return true;
    }

    /**
     * @brief Detener y desvincular un socket activo.
     */
    bool removeReceiver(short port) {
        auto socket = receivers_.find(port);

        if (socket == receivers_.end()){
            std::cerr << "No se ha encontrado el socket con puerto" << port << std::endl;
            return false;
        }

        socket->second->stop();
        receivers_.erase(socket);

        std::cout << "NetMgr: Socket removed from port " << port << "\n";

        return true;
    }

    /**
     * @brief Inicia un número de hilos con el contexto de operaciones asíncronas.
     * @details Toma por defecto el número máximo permitido por el sistema.
     * Esto no significa que no se puedan hacer más hilos, porque se quedan "idle" 
     * esperando datos en los sockets.
     */
    void start() {
        for (std::size_t i = 0; i < thread_count_; ++i) {
            threads_.emplace_back([this]() {
                io_context_.run();
            });
        }
    }

    /**
     * @brief Detiene todos los sockets.
     */
    void stop() {
        work_guard_.reset();   // permite que run() termine
        io_context_.stop();

        for (auto& t : threads_) {
            if (t.joinable())
                t.join();
        }

        threads_.clear();
    }

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