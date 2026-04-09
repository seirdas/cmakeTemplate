#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <miniaudio.h>

// General ------------------------------------------------------------------------------

AppController::AppController() : ui_(this), running_(false) {

}

AppController::~AppController() {

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    // Cerrar sockets
    std::cout << "[AppController] Closing sockets and network I/O..." << std::endl;
    net_.stop();

    // Cerrar el worker esperando el paquete online
    online_cv_.notify_all();
    std::cout << "[AppController] Closing running threads..." << std::endl;
    if (worker_.joinable())
        worker_.join();


    /* TODO ESTO ES OPCIONAL PORQUE FORMA PARTE DE LOS DESTRUCTORES DE LAS CLASES */
    
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
    running_ = true;

    // Iniciar sockets
    net_.start();

    // Inicialización de audio
    if (!snd_.init()) return false;

    // Inicialización de TTS
    if (!tts_.init()) return false;
    
    // TODO: esta comprobación la deberia hacer soundmgr
    std::vector<std::string> devices = snd_.getAvailablePlaybacks();
    if (!devices.empty()) {
        snd_.addPlaybackDevice(devices[0], "audio"); 
    } else {
        std::cerr << "[AppController] WARN No se encontraron altavoces en el PC." << std::endl;
    }

    // Inicialización de ventana UI
    if (!ui_.init())         return false;

    // Inicialización de sockets
    net_.addReceiver("Host",    8080);
    net_.addReceiver("Other",   12345,  "127.0.0.1", sizeof(unsigned long long));
    net_.addReceiver("Other2",  12345,  "127.0.0.1", 8765);
    net_.addReceiver("Other",   12225,  "127.0.0.1");
    net_.addReceiver("TTS",     1345,   "127.0.0.1", 5076);
    net_.removeReceiver(net_.getSocketIndex("TTS"));

    net_.printReceivers();
    
    // Hilo consumidor de paquetes Host
    worker_ = std::thread(&AppController::TWorker, this);

    /*else*/
    return true;
}

int AppController::run() {

    // prueba TTS
    std::string text = "bottle of water.";
    tts_.generate(text, "./ttstest");

    ui_.run(); // Bloquea hasta cerrar
    return 0;
}


// Hilos --------------------------------------------------------------------------------

void AppController::TWorker() {
    std::cout << "[TWorker]   Initializating consumer thread..." << std::endl;
    std::vector<char> data;

    while (running_) {

        std::unique_lock<std::mutex> lock(online_mtx_);

        online_cv_.wait(lock, [this] {
            return !running_ || mode_ == AppMode::ONLINE;
        });

        if (!running_) break;

        lock.unlock();  // Libera el lock de aquí en adelante

        // Pide datos al socket para procesar
        int index = net_.getSocketIndex("Host");

        if (index == -1) {
            std::cerr << "[TWorker] ERROR 'Host' Socket undefined" << std::endl;
            return; // <- TODO reiniciar o hacer algo para recuperarse de esto si pasa
        }

        data = net_.getDataFromSocket(index);

        if (data.empty()) {
            std::cerr << "[TWorker] Empty data received" << std::endl;
            continue;
        }

        // Procesar el paquete (simulado)
        std::cout << "[TWorker]   Procesando paquete de datos..." << std::endl;
        std::cout << "Size of data" << data.size() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }

    std::cout << "[TWorker]   Consumer thread stopped." << std::endl;
}


// IAppControl methods ------------------------------------------------------------
    
    // Aplicación -----------------------------------------------------------------------

    std::string AppController::getVersion() const noexcept { 
        return version_; 
    }

    void AppController::setOnlineMode(bool b_modo) noexcept { 

        std::unique_lock<std::mutex> lock(online_mtx_);
        AppMode nuevoModo = (b_modo) ? AppMode::ONLINE : AppMode::OFFLINE;
        if (mode_ == nuevoModo) return;
        mode_ = nuevoModo;
        lock.unlock();

        switch (mode_){
            case AppMode::ONLINE:
                std::cout << "[AppController]   Switching to ONLINE..." << std::endl;
                net_.start(); 
                online_cv_.notify_all();
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
        // TODO
        return true;
    }

    bool AppController::removeReceiver() const noexcept {
        // TODO
        return true;
    }
