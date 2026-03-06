#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <miniaudio.h>


/********************************/
// ----- APPCONTROLLER ----------/
/********************************/

// General ------------------------------------------------------------------------------

AppController::AppController() : ui_(this) {

}

AppController::~AppController() {
    
    // Cerrar sockets
    std::cout << "[AppController] Closing sockets and network I/O..." << std::endl;
    net_.stop();

    // Cerrar módulo de sonido
    std::cout << "[AppController] Closing sound module..." << std::endl;
    snd_.stop();
    
    // Cerrar ventana UI
    std::cout << "[AppController] Closing UI..." << std::endl;
    ui_.cerrar();

}

bool AppController::init(){

    // Inicialización de sockets
    net_.addReceiver(8080);
    net_.addReceiver(12345, "127.0.0.1", sizeof(unsigned long long));

    // Inicialización de audio
    if (!snd_.init())
        return false;

    // Inicialización de ventana UI
    if (!ui_.init()) 
        return false;

    /*else*/
    return true;
}

int AppController::run() {

    
    // Iniciar sockets
    net_.start();

    // Gestor de paquetes
    //worker_ = std::thread(&AppController::TWorker, this);

    // Audio test

    //snd_.StartRec("prueba2");

    // Ventana UI
    ui_.run();      // <-- Este método bloquea hasta que la ventana se cierre

    //snd_.StopRec();

    return 0;
}

void AppController::TWorker() {

    std::cout << "[AppController] Initializating consumer thread..." << std::endl;

    while (net_.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[AppController] Consumer thread stopped." << std::endl;

}


// IAppControl methods ------------------------------------------------------------

std::string AppController::getVersion() const noexcept { 
    return version_; 
}

int AppController::get_SocketPort() const noexcept { 
    return 1; 
}

void AppController::setMode(bool b_modo) noexcept { 

    AppMode nuevoModo = (b_modo) ? AppMode::ONLINE : AppMode::OFFLINE;
    if (mode_ == nuevoModo) return;
    mode_ = nuevoModo;

    switch (mode_){
        case AppMode::ONLINE:
            std::cout << "[AppController] Switching to ONLINE..." << std::endl;
            net_.start(); 
        break;
        case AppMode::OFFLINE:
            std::cout << "[AppController] Switching to OFFLINE..." << std::endl;
            net_.stop(); // Esto cierra sockets y libera el getFirstPacket() bloqueado
        break;
        default:
            // No debería llegar aquí nunca
            std::cerr << "[AppController] Unknown state mode" << std::endl;
    }
};

bool AppController::getMode() const noexcept {
    switch (mode_){
        case AppMode::ONLINE:   return true;
        case AppMode::OFFLINE:  return false;
    }
    // No debería llegar aquí nunca
    return true;
};