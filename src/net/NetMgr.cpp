#include <iostream>             // Entrada/Salida estándar
#include "net/NetMgr.hpp"
#include "net/UdpReceiver.hpp"

// General ------------------------------------------------------------------------------

NetMgr::NetMgr(std::size_t const& thread_count)
    : work_guard_(asio::make_work_guard(io_context_)),
        thread_count_(thread_count == 0 ? 1 : thread_count)
{

}

NetMgr::~NetMgr() {
    stop();
}

void NetMgr::printReceivers() {

    std::cout << "\n--- SOCKETS ACTIVOS ---" << std::endl;
    for (unsigned int i = 0; i<receivers_.size(); i++) {
        std::cout << "[" << i << "] Socket '" << receivers_[i]->name() << "'    | port:" << receivers_[i]->port() << std::endl;
    }
    std::cout << std::endl;

}

bool NetMgr::addReceiver(
    std::string         name,
    short               local_port, 
    const std::string&  local_ip, 
    unsigned int        rcv_packet_size
) 
{
    // Evitar duplicados por nombre y puerto
    for (const auto& rcv : receivers_) {
        if (rcv->name() == name) {
            std::cerr << "[NetMgr]  Socket already exists with name " << name << std::endl;
            return false;
        }
        if (rcv->port() == local_port) {
            std::cerr << "[NetMgr]  Socket already exists with port " << local_port << std::endl;
            return false;
        }
    }

    // Crear receiver (aún no registrado)
    std::shared_ptr<UdpReceiver> receiver = std::make_shared<UdpReceiver>(name, io_context_);

    // Intentar inicializar
    std::cout << "[NetMgr]  Opening new socket..." << std::endl;
    if (!receiver->init(local_port, local_ip, rcv_packet_size))
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        std::cerr << "[NetMgr]  Failed to initialize socket on port " << local_port << std::endl;
        return false;
    }

    // Insertar en el vector
    receivers_.push_back(receiver);
    std::cout << "[NetMgr]  Socket added on port " << local_port << "\n";

    return true;
}

bool NetMgr::removeReceiver(unsigned int index) {
    // 1. Lock del mutex si lo has añadido (muy recomendado)
    std::lock_guard<std::mutex> lock(mtx_receivers_);

    if (index >= receivers_.size()) {
        std::cerr << "[NetMgr]  Selected index " << index;
        std::cerr << " out of bounds (" << receivers_.size() <<")" << std::endl;
        return false;
    }

    std::cout << "[NetMgr] Deleting socket '"
              << receivers_[index]->name()
              << "' with port "
              << receivers_[index]->port()
              << std::endl;
              
    // Esto fuerza a que el lambda de async_receive_from se ejecute con error 'operation_aborted'.
    receivers_[index]->stop(); 

    receivers_.erase(receivers_.begin() + index);

    std::cout << "[NetMgr] Socket liberado del registro." << std::endl;
    return true;
}

int NetMgr::getSocketIndex(short port) const {
    for (unsigned int i = 0; i < receivers_.size(); i++)
        if (receivers_[i]->port() == port) return i;
    /*else*/ return -1;
}

int NetMgr::getSocketIndex(std::string const& name) const {
    for (unsigned int i = 0; i < receivers_.size(); i++)
        if (receivers_[i]->name() == name) return i;
    /*else*/ return -1;
}


std::vector<char> NetMgr::getDataFromSocket(unsigned int index) {

    if (index >= receivers_.size()) {
        std::cerr << "[NetMgr]  ERROR Selected index out of limits. " << std::endl;
        return {};
    }

    return receivers_[index]->getFirstPacket();   // <-- BLOQUEANTE 
}

void NetMgr::start() {
    // Evitar lanzar hilos si ya están corriendo
    if (isRunning()) {
        std::cout << "[NetMgr]  Already running." << std::endl;
        return;
    }

    std::cout << "[NetMgr]  Starting I/O context with " << thread_count_ << " threads..." << std::endl;
    
    // Si el contexto fue detenido previamente (stop), hay que reiniciarlo antes de run()
    if (io_context_.stopped()) {
        io_context_.restart();
    }

    // Iniciar (de nuevo si aplica) los sockets
    for (auto& rcv : receivers_) {
        asio::post(io_context_, [rcv]{ /*TODO*/ });
        rcv->clearCache();
    }

    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }

    running_ = true;
}

void NetMgr::stop() {

    if (!isRunning()) return;

    std::cout << "[NetMgr]  Stopping network manager..." << std::endl;

    // Parar la recepción de los sockets y limpiar
    for (auto& rcv : receivers_) {
        asio::post(io_context_, [rcv]{ rcv->stop(); });
        rcv->clearCache();
    }

    // Parar el io_context
    work_guard_.reset();   // permite que run() termine
    io_context_.stop();

    // Esperar a que terminen las tareas los receptores IO
    for (auto& t : threads_) {
        if (t.joinable())
            t.join();
    }

    threads_.clear();

    running_ = false;
    std::cout << "[NetMgr]  All threads stopped." << std::endl;
}

bool NetMgr::isRunning() const {
    return running_;
}
