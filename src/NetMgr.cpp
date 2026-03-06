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
    std::string         name,
    short               local_port, 
    const std::string&  local_ip, 
    unsigned int        rcv_packet_size
) 
{
    // Evitar duplicados por nombre y puerto
    for (const auto& [n, sock] : receivers_) {
        if (n == name) {
            std::cerr << "[NetMgr]  Socket already exists with name " << name << std::endl;
            return false;
        }
        if (sock->port() == local_port) {
            std::cerr << "[NetMgr]  Socket already exists with port " << local_port << std::endl;
            return false;
        }
    }

    // Crear receiver (aún no registrado)
    std::unique_ptr<UdpReceiver> receiver = std::make_unique<UdpReceiver>(name, io_context_);

    // Intentar inicializar
    std::cout << "[NetMgr]  Opening new socket..." << std::endl;
    if (!receiver->init(local_port, local_ip, rcv_packet_size))
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        std::cerr << "[NetMgr]  Failed to initialize socket on port " << local_port << std::endl;
        return false;
    }

    // Insertar en el vector
    receivers_.emplace(name, std::move(receiver));
    std::cout << "[NetMgr]  Socket added on port " << local_port << "\n";

    return true;
}

bool NetMgr::removeReceiver(short port) {

    for (auto it = receivers_.begin(); it != receivers_.end(); ++it)
        if (it->second->port() == port) {
            it->second->stop();
            std::cout << "[NetMgr] Deleting socket '"
                    << it->second->name()
                    << "' with port "
                    << it->second->port()
                    << std::endl; 
            receivers_.erase(it);
            std::cout << "[NetMgr] Deleted socket" << std::endl;
            return true;
        }

    /*else*/ 
    std::cerr << "[NetMgr]  ERROR Socket with port '" << port << "' not found." << std::endl;
    return false;
}

bool NetMgr::removeReceiver(const std::string& name) {

    auto it = receivers_.find(name);

    if (it == receivers_.end()) {
        std::cerr << "[NetMgr] ERROR Socket with name '" 
                  << name << "' not found." << std::endl;
        return false;
    }

    it->second->stop();
    std::cout << "[NetMgr] Deleting socket '"
              << it->second->name()
              << "' with port "
              << it->second->port()
              << std::endl; 
    receivers_.erase(it);
    std::cout << "[NetMgr] Deleted socket" << std::endl;

    return true;
}

std::vector<char> NetMgr::getDataFromSocket(const std::string& name) {
    auto it = receivers_.find(name);

    if (it == receivers_.end()) {
        std::cerr << "[NetMgr] ERROR Socket with name '" 
                  << name << "' not found." << std::endl;
        return {};
    }

    return it->second->getFirstPacket();            // <-- BLOQUEANTE 
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
    std::cout << "[NetMgr]  Stopping network manager..." << std::endl;

    work_guard_.reset();   // permite que run() termine
    io_context_.stop();

    // Borrar la cola de datos de los receptores
    for (auto const& it : receivers_){
        it.second->clearCache();
    }

    // Esperar a que terminen las tareas los receptores
    for (auto& t : threads_) {
        if (t.joinable())
            t.join();
    }

    threads_.clear();
    std::cout << "[NetMgr]  All threads stopped." << std::endl;
}

bool NetMgr::isRunning() {
    // Es considerado activo si el contexto no está detenido y hay hilos trabajando
    return !io_context_.stopped() && !threads_.empty();
}
