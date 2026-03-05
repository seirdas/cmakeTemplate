#include <iostream>             // Entrada/Salida estándar
#include "NetMgr.hpp"

// General ------------------------------------------------------------------------------

NetMgr::NetMgr(std::size_t thread_count)
    : work_guard_(asio::make_work_guard(io_context_)),
        thread_count_(thread_count == 0 ? 1 : thread_count)
{

}

NetMgr::~NetMgr() {
    stop();
}

bool NetMgr::addReceiver(
    short               local_port, 
    const std::string&  local_ip, 
    unsigned int        rcv_packet_size
) 
{

    // Evitar duplicados
    if (receivers_.find(local_port) != receivers_.end()) {
        std::cerr << "[NetMgr]  Socket already exists on port " << local_port << std::endl;
        return false;
    }

    // Crear receiver (aún no registrado)
    auto receiver = std::make_unique<UdpReceiver>(io_context_);

    // Intentar inicializar
    std::cout << "[NetMgr]  Opening new socket..." << std::endl;
    if (!receiver->init(local_port, local_ip, rcv_packet_size))
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        std::cerr << "[NetMgr]  Failed to initialize socket on port " << local_port << std::endl;
        return false;
    }

    // Insertar en el vector
    receivers_.emplace(local_port, std::move(receiver));
    std::cout << "[NetMgr]  Socket added on port " << local_port << "\n";

    return true;
}

bool NetMgr::removeReceiver(short port) {

    std::cout << "[NetMgr]  Trying to remove socket with port " << port << std::endl;
    auto socket = receivers_.find(port);
    if (socket == receivers_.end()){
        std::cerr << "[NetMgr]  Socket with port " << port << " not found." << std::endl;
        return false;
    }

    socket->second->stop();
    receivers_.erase(socket);

    std::cout << "[NetMgr]  Socket removed with port " << port << "\n";

    return true;
}

void NetMgr::start() {
    std::cout << "[NetMgr]  Init I/O context..." << std::endl;
    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
}

void NetMgr::stop() {
    work_guard_.reset();   // permite que run() termine
    io_context_.stop();

    for (auto& t : threads_) {
        if (t.joinable())
            t.join();
    }

    threads_.clear();
}
