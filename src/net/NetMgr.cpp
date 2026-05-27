#include "net/NetMgr.hpp"
#include "net/UdpSocket.hpp"
#include "system/SystemMgr.hpp"

// General ------------------------------------------------------------------------------

NetMgr::NetMgr(std::size_t const& thread_count) :
    work_guard_(asio::make_work_guard(io_context_)),
    io_running_(false),
    sockets_running_(false),
    thread_count_(thread_count == 0 ? 1 : thread_count)
{

}

NetMgr::~NetMgr() {
    stop(); // Para los sockets

    // Quitar protector para que los hilos pueden salir de run()
    work_guard_.reset();

    // (Opcional) Fuerza la parada de cualquier handler pendiente del io_context
    io_context_.stop();  
    for (auto& t : threads_)
        if (t.joinable()) t.join(); // Esperar a los hilos de io_context
    io_running_ = false;
    threads_.clear();
}

void NetMgr::printReceivers() {

    SYS_INFO("NetMgr", "--- SOCKETS ACTIVOS ---");
    for (unsigned int i = 0; i < udp_sockets_.size(); i++) {
        // SYS_INFO("NetMgr", "[" + std::to_string(i) + "] Socket '" + udp_sockets_[i]->name() + "'    | port:" + udp_sockets_[i]->port() );
        SYS_INFO("NetMgr", "[" + std::to_string(i) + "] Socket '" + udp_sockets_[i]->name() + ":" + std::to_string(udp_sockets_[i]->port() ) + "'");
    }

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
    for (const auto& rcv : udp_sockets_) {
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
    std::shared_ptr<UdpSocket> receiver = std::make_shared<UdpSocket>(name, io_context_);

    // Intentar inicializar
    SYS_INFO("NetMgr","Opening new socket...");
    if (!receiver->init(local_port, local_ip, rcv_packet_size))
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        SYS_WARN("NetMgr","Failed to initialize socket on port " + std::to_string(local_port));
        return false;
    }

    // Insertar en el vector
    std::lock_guard<std::mutex> lock(mtx_udp_sockets_);
    udp_sockets_.push_back(receiver);
    SYS_INFO("NetMgr","Socket added on port " + std::to_string(local_port) );

    // "encender" el socket si estaban los demás corriendo
    if (sockets_running_)
        asio::post(receiver->getStrand(), [receiver]{ 
            receiver->start(); 
        });


    return true;
}

bool NetMgr::removeReceiver(unsigned int index) {
    std::lock_guard<std::mutex> lock(mtx_udp_sockets_);

    if (index >= udp_sockets_.size()) {
        SYS_WARN("NetMgr","Selected index " + std::to_string(index) 
            + " out of bounds (" + std::to_string(udp_sockets_.size()) +")");
        return false;
    }

    // Copiamos el shared_ptr para mantenerlo vivo durante esta función
    std::shared_ptr<UdpSocket> rcv = udp_sockets_[index];
    
    SYS_INFO("NetMgr",
        "Deleting socket '" + rcv->name()
        + "' with port " + std::to_string(rcv->port() )
    );
              
    // Esto fuerza a que el lambda de async_receive_from se ejecute con error 'operation_aborted'.
    asio::post(rcv->getStrand(), [rcv]() {
        rcv->stop();
    });

    // Borrar el elemento del vector usando iterator
    udp_sockets_.erase(udp_sockets_.begin() + index);

    SYS_INFO("NetMgr","Socket deleted");
    return true;
}

// Datos de sockets ---------------------------------------------------------------------

void NetMgr::sendData(
    std::string socketname, 
    const std::vector<char>& data,
    const std::string& dest_ip,
    unsigned short dest_port
) 
{
    // Si hay un socket con ese nombre, manda los datos
    bool exist = false;
    for (const auto& rcv : udp_sockets_) {
        if (rcv->name() == socketname) {
            rcv->sendPacket(data, dest_ip, dest_port);
            exist = true;
            return;
        }
    }

    // Llegará aquí si no encuentra socket
    SYS_WARN("NetMgr","Socket not exists with name " + socketname);
}

int NetMgr::getSocketIndex(short port) const {
    for (unsigned int i = 0; i < udp_sockets_.size(); i++)
        if (udp_sockets_[i]->port() == port) return i;
    /*else*/ return -1;
}

int NetMgr::getSocketIndex(std::string const& name) const {
    for (unsigned int i = 0; i < udp_sockets_.size(); i++)
        if (udp_sockets_[i]->name() == name) return i;
    /*else*/ return -1;
}

std::vector<char> NetMgr::getDataFromSocket(unsigned int index) {

    if (index >= udp_sockets_.size()) {
        SYS_WARN("NetMgr","Selected index out of limits");
        return {};
    }

    return udp_sockets_[index]->getFirstPacket();   // <-- BLOQUEANTE 
}

// Ejecución ----------------------------------------------------------------------------

bool NetMgr::start() {
    // Evitar lanzar hilos si ya están corriendo
    if (sockets_running_) {
        SYS_WARN("NetMgr","Commanded start() when is already running");
        return true;
    }

    // Inicializar el io_context con varios hilos de recepción
    if (!io_running_) {
        io_context_.restart();
        SYS_INFO("NetMgr","Starting I/O context with " + std::to_string(thread_count_) + " threads...");
        for (std::size_t i = 0; i < thread_count_; ++i) {
            threads_.emplace_back([this]() {
                io_context_.run();
            });
        }
        SYS_INFO("NetMgr","Network (io_context) running...");
        io_running_=true;
    }

    // Iniciar (de nuevo si aplica) los sockets
    for (auto& rcv : udp_sockets_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->start(); 
        });
    }

    SYS_INFO("NetMgr","Sockets running");
    sockets_running_ = true;
    return true;
}

void NetMgr::stop() {

    if (!sockets_running_) return;

    SYS_INFO("NetMgr","Stopping sockets...");

    // Parar la recepción de los sockets
    for (auto& rcv : udp_sockets_) {
        asio::post(rcv->getStrand(), [rcv]{ 
            rcv->stop(); 
        });
    }

    sockets_running_ = false;
    SYS_INFO("NetMgr","All receivers stopped");
}

bool NetMgr::isRunning() const {
    return sockets_running_;
}
