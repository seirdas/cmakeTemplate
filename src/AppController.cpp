#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <miniaudio.h>

// General ------------------------------------------------------------------------------

AppController::AppController() : ui_(this), isRunning_(false) {

}

AppController::~AppController() {

    // Notifica el estado de cerrado (para threads, etc.)
    isRunning_ = false;

    // Esperar a que terminen los hilos
    std::cout << "[AppController] Closing running threads..." << std::endl;
    if (worker_.joinable())
        worker_.join();


    /* TODO ESTO ES OPCIONAL PORQUE FORMA PARTE DE LOS DESTRUCTORES DE LAS CLASES */
    
    // Cerrar sockets
    std::cout << "[AppController] Closing sockets and network I/O..." << std::endl;
    net_.stop();

    // Cerrar módulo de sonido
    std::cout << "[AppController] Closing sound module..." << std::endl;
    snd_.stop();
    
    // Cerrar ventana UI
    std::cout << "[AppController] Closing UI..." << std::endl;
    ui_.cerrar();

    std::cout << "[AppController] Exiting..." << std::endl;
}

bool AppController::init() {

    // Flag indicando la ejecución
    isRunning_ = true;

    
    // Iniciar sockets
    net_.start();

    // Inicialización de audio
    if (!snd_.init())        return false;

    // Inicialización de ventana UI
    if (!ui_.init())         return false;

    // Inicialización de sockets
    net_.addReceiver("Host", 8080);
    net_.addReceiver("Other", 12345,    "127.0.0.1", sizeof(unsigned long long));
    net_.addReceiver("Other2", 12345,   "127.0.0.1", sizeof(unsigned long long));
    net_.addReceiver("Other", 12225,    "127.0.0.1", sizeof(unsigned long long));
    net_.addReceiver("jose", 1345,      "127.0.0.1", sizeof(unsigned long long));

    net_.removeReceiver(net_.getSocketIndex("Host"));

    net_.printReceivers();

    /*else*/
    return true;
}

int AppController::run() {


    // Gestor de paquetes (#TODO)
    //worker_ = std::thread(&AppController::TWorker, this);

    // Sonido
    //snd_.playbackTest();

    // Ventana UI
    ui_.run();      // <-- Este método bloquea hasta que la ventana se cierre

    return 0;
}


// Hilos --------------------------------------------------------------------------------

void AppController::TWorker() {

    std::cout << "[AppController]   Initializating consumer thread..." << std::endl;

    std::vector<char> data;

    while (isRunning_) {
        switch (mode_){
            case AppMode::ONLINE:
                // Esperar a que net le de algo para procesar
                data = net_.getDataFromSocket(net_.getSocketIndex("Host"));
            break;
            case AppMode::OFFLINE:
                // Esperar a que la UI le de algo para procesar
                // #TODO
            break;
            default:
                // No debería llegar aquí nunca
                std::cerr << "[AppController]   ERROR Undefined AppMode in consumer thread" << std::endl;
        }

        if (!isRunning_) break;

        // Procesar el paquete (simulado)
        std::cout << "[AppController]   Procesando paquete de datos..." << std::endl;
        std::cout << "Size of data" << data.size() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }

    std::cout << "[AppController]   Consumer thread stopped." << std::endl;

}


// IAppControl methods ------------------------------------------------------------
    
    // Aplicación -----------------------------------------------------------------------

    std::string AppController::getVersion() const noexcept { 
        return version_; 
    }

    void AppController::setOnlineMode(bool b_modo) noexcept { 

        AppMode nuevoModo = (b_modo) ? AppMode::ONLINE : AppMode::OFFLINE;
        if (mode_ == nuevoModo) return;
        mode_ = nuevoModo;

        switch (mode_){
            case AppMode::ONLINE:
                std::cout << "[AppController]   Switching to ONLINE..." << std::endl;
                net_.start(); 
            break;
            case AppMode::OFFLINE:
                std::cout << "[AppController]   Switching to OFFLINE..." << std::endl;
                net_.stop(); // Esto cierra sockets y libera el getFirstPacket() bloqueado
            break;
            default:
                // No debería llegar aquí nunca
                std::cerr << "[AppController]   ERROR Unknown state mode" << std::endl;
        }
    };

    bool AppController::isOnlineMode() const noexcept {
        switch (mode_){
            case AppMode::ONLINE:   return true;
            case AppMode::OFFLINE:  return false;
        }
        // No debería llegar aquí nunca
        std::cerr << "[AppController]   ERROR Undefined AppMode." << std::endl;
        return true;
    };

    
    // Audio ----------------------------------------------------------------------------

    std::vector<std::string> AppController::getAvailableInputDevices() noexcept {
        return snd_.getAvailableInputs();
    }

    std::vector<std::string> AppController::getAvailablePlaybackDevices() noexcept {
        return snd_.getAvailablePlaybacks();
    }

    
    // Sockets --------------------------------------------------------------------------
    
    bool AppController::addReceiver() const noexcept {
        return true;
    }

    bool AppController::removeReceiver() const noexcept {
        return true;
    }
