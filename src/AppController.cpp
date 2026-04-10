#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <miniaudio.h>

// General ------------------------------------------------------------------------------

AppController::AppController() : 
    gui_(this), 
    net_initialized_(false),
    gui_initialized_(false),
    snd_initialized_(false),
    tts_initialized_(false),
    running_(false),
    online_mode_(true),
    version_(std::to_string(VERSION))
{

}

AppController::~AppController() {

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    // Cerrar socket y worker esperando paquetes de red
    net_.stop();
    online_cv_.notify_all();
    std::cout << "[AppController] Closing running threads..." << std::endl;
    if (worker_.joinable())
        worker_.join();

    // Cerrar módulos (opcional)
    snd_.stop();
    gui_.cerrar();
    tts_.cerrar();
}

bool AppController::init() {

    // Iniciar módulos
    net_initialized_ = net_.start();
    snd_initialized_ = snd_.init();
    gui_initialized_ = gui_.init();

    // Inicialización de TTS (en hilo para no bloquear)
    std::thread tLoadTTS([this]() {
            tts_initialized_ = tts_.init();
        }
    );
    tLoadTTS.detach();  // No necesitamos "esperar" a que termine
    
    // Hilo consumidor de paquetes Host
    worker_ = std::thread(&AppController::TWorker, this);

    /*else*/
    return true;
}

int AppController::run() {
    running_ = true;
    gui_.run(); // Bloquea hasta cerrar
    return 0;
}


// Hilos --------------------------------------------------------------------------------
void AppController::TWorker() {
    std::cout << "[TWorker]   Initializating consumer thread..." << std::endl;
    std::vector<char> data;

    while (running_) {

        std::unique_lock<std::mutex> lock(online_mtx_);

        online_cv_.wait(lock, [this] {
            return !running_ || online_mode_;
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

    void AppController::setOnlineMode(bool nuevo_online_mode) noexcept { 

        std::unique_lock<std::mutex> lock(online_mtx_);
        if(online_mode_==nuevo_online_mode) return;
        online_mode_=nuevo_online_mode;
        lock.unlock();

        if(online_mode_) {
            net_.start();
            online_cv_.notify_all();
        }
        else
            net_.stop();
    };

    bool AppController::isOnlineMode() const noexcept {
        return online_mode_;
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
