#include <iostream>             // Entrada/Salida estándar
#include "net/NetMgr.hpp"
#include "net/UdpReceiver.hpp"

// General ------------------------------------------------------------------------------

NetMgr::NetMgr(std::size_t const& thread_count) :
    work_guard_(asio::make_work_guard(io_context_)),
    running_(false),
    thread_count_(thread_count == 0 ? 1 : thread_count)
{
    // Inicializar el io_context con varios hilos de recepción
    io_context_.restart();
    std::cout << "[NetMgr]  Starting I/O context with " << thread_count_ << " threads..." << std::endl;
    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    std::cout << "[NetMgr]  Network (io_context) running..." << std::endl;
}

NetMgr::~NetMgr() {
    stop(); // Para los sockets

    // Quitar protector para que los hilos pueden salir de run()
    work_guard_.reset();

    // (Opcional) Fuerza la parada de cualquier handler pendiente
    io_context_.stop();  
    for (auto& t : threads_)
        if (t.joinable()) t.join(); // Esperar a los hilos de io_context
    threads_.clear();
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
    std::lock_guard<std::mutex> lock(mtx_receivers_);
    receivers_.push_back(receiver);
    std::cout << "[NetMgr]  Socket added on port " << local_port << std::endl;

    // "encender" el socket si estaban los demás corriendo
    if (running_)
        asio::post(receiver->getStrand(), [receiver]{ 
            receiver->start(); 
        });


    return true;
}

bool NetMgr::removeReceiver(unsigned int index) {
    std::lock_guard<std::mutex> lock(mtx_receivers_);

    if (index >= receivers_.size()) {
        std::cerr << "[NetMgr]  Selected index " << index;
        std::cerr << " out of bounds (" << receivers_.size() <<")" << std::endl;
        return false;
    }

    // Copiamos el shared_ptr para mantenerlo vivo durante esta función
    std::shared_ptr<UdpReceiver> rcv = receivers_[index];
    
    std::cout << "[NetMgr] Deleting socket '"
              << rcv->name()
              << "' with port "
              << rcv->port()
              << std::endl;
              
    // Esto fuerza a que el lambda de async_receive_from se ejecute con error 'operation_aborted'.
    asio::post(rcv->getStrand(), [rcv]() {
        rcv->stop();
    });

    // Borrar el elemento del vector usando iterator
    receivers_.erase(receivers_.begin() + index);

    std::cout << "[NetMgr]  Socket deleted" << std::endl;
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
        std::cerr << "[NetMgr]  ERROR Selected index out of limits" << std::endl;
        return {};
    }

    return receivers_[index]->getFirstPacket();   // <-- BLOQUEANTE 
}

bool NetMgr::start() {
    // Evitar lanzar hilos si ya están corriendo
    if (running_) {
        std::cout << "[NetMgr]  Already running" << std::endl;
        return true;
    }

    // Iniciar (de nuevo si aplica) los sockets
    for (auto& rcv : receivers_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->start(); 
        });
    }

    std::cout << "[NetMgr]  Sockets running" << std::endl;
    running_ = true;
    return true;
}

void NetMgr::stop() {

    if (!running_) return;

    std::cout << "[NetMgr]  Stopping sockets..." << std::endl;

    // Parar la recepción de los sockets
    for (auto& rcv : receivers_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->stop(); 
        });
    }

    running_ = false;
    std::cout << "[NetMgr]  All receivers stopped" << std::endl;
}

bool NetMgr::isRunning() const {
    return running_;
}
