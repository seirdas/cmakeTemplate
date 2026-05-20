#include <iostream>             // Entrada/Salida estándar
#include "net/NetMgr.hpp"
#include "net/UdpReceiver.hpp"
#include "system/SystemMgr.hpp"

// General ------------------------------------------------------------------------------

NetMgr::NetMgr(std::size_t const& thread_count) :
    work_guard_(asio::make_work_guard(io_context_)),
    running_(false),
    thread_count_(thread_count == 0 ? 1 : thread_count)
{
    // Inicializar el io_context con varios hilos de recepción
    io_context_.restart();
    SYS_INFO("NetMgr","Starting I/O context with " + std::to_string(thread_count_) + " threads...");
    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    SYS_INFO("NetMgr","Network (io_context) running...");
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

// Gestión de sockets -------------------------------------------------------------------

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
            SYS_WARN("NetMgr","Socket already exists with name " + name);;
            return false;
        }
        if (rcv->port() == local_port) {
            SYS_WARN("NetMgr","Socket already exists with port " + std::to_string(local_port));
            return false;
        }
    }

    // Crear receiver (aún no registrado)
    std::shared_ptr<UdpReceiver> receiver = std::make_shared<UdpReceiver>(name, io_context_);

    // Intentar inicializar
    SYS_INFO("NetMgr","Opening new socket...");
    if (!receiver->init(local_port, local_ip, rcv_packet_size))
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        SYS_WARN("NetMgr","Failed to initialize socket on port " + std::to_string(local_port));
        return false;
    }

    // Insertar en el vector
    std::lock_guard<std::mutex> lock(mtx_receivers_);
    receivers_.push_back(receiver);
    SYS_INFO("NetMgr","Socket added on port " + std::to_string(local_port) );;

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
        SYS_WARN("NetMgr","Selected index " + std::to_string(index) 
            + " out of bounds (" + std::to_string(receivers_.size()) +")");
        return false;
    }

    // Copiamos el shared_ptr para mantenerlo vivo durante esta función
    std::shared_ptr<UdpReceiver> rcv = receivers_[index];
    
    SYS_INFO("NetMgr",
        "Deleting socket '" + rcv->name()
        + "' with port " + std::to_string(rcv->port() )
    );
              
    // Esto fuerza a que el lambda de async_receive_from se ejecute con error 'operation_aborted'.
    asio::post(rcv->getStrand(), [rcv]() {
        rcv->stop();
    });

    // Borrar el elemento del vector usando iterator
    receivers_.erase(receivers_.begin() + index);

    SYS_INFO("NetMgr","Socket deleted");
    return true;
}

// Datos de sockets ---------------------------------------------------------------------

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
        SYS_WARN("NetMgr","Selected index out of limits");
        return {};
    }

    return receivers_[index]->getFirstPacket();   // <-- BLOQUEANTE 
}

// Ejecución ----------------------------------------------------------------------------

bool NetMgr::start() {
    // Evitar lanzar hilos si ya están corriendo
    if (running_) {
        SYS_WARN("NetMgr","Commanded start() when is already running");
        return true;
    }

    // Iniciar (de nuevo si aplica) los sockets
    for (auto& rcv : receivers_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->start(); 
        });
    }

    SYS_INFO("NetMgr","Sockets running");
    running_ = true;
    return true;
}

void NetMgr::stop() {

    if (!running_) return;

    SYS_INFO("NetMgr","Stopping sockets...");

    // Parar la recepción de los sockets
    for (auto& rcv : receivers_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->stop(); 
        });
    }

    running_ = false;
    SYS_INFO("NetMgr","All receivers stopped");
}

bool NetMgr::isRunning() const {
    return running_;
}
