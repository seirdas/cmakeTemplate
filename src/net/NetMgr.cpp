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

    if (index > receivers_.size()) {
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

              
    // Detener socket
    rcv->stop();

    // Borrar el elemento del vector usando iterator
    receivers_.erase(receivers_.begin() + index);

    std::cout << "[NetMgr] Deleted socket" << std::endl;
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

    if (index > receivers_.size()) {
        std::cerr << "[NetMgr]  Selected index out of limits. " << std::endl;
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

    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
}

void NetMgr::stop() {

    if (!isRunning()) return;

    std::cout << "[NetMgr]  Stopping network manager..." << std::endl;

    work_guard_.reset();   // permite que run() termine
    io_context_.stop();

    // Borrar la cola de datos de los receptores
    for (auto const& rcv : receivers_){
        rcv->clearCache();
    }

    // Esperar a que terminen las tareas los receptores
    for (auto& t : threads_) {
        if (t.joinable())
            t.join();
    }

    threads_.clear();
    std::cout << "[NetMgr]  All threads stopped." << std::endl;
}

bool NetMgr::isRunning() const {
    // Es considerado activo si el contexto no está detenido y hay hilos trabajando
    return !io_context_.stopped() && !threads_.empty();
}
