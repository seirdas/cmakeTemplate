#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <miniaudio.h>
#include <iostream>
#include "system/sys.hpp"

// General ------------------------------------------------------------------------------

AppController::AppController() : 
    gui_(this),
    log_("log.log"),
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
    SYS_INFO("AppController","Closing running threads...");
    if (worker_.joinable())
        worker_.join();

    // Cerrar módulos (opcional)
    snd_.stop();
    gui_.cerrar();
    tts_.cerrar();
}

bool AppController::init() {

    // Iniciar módulos
    gui_initialized_ = gui_.init();
    net_initialized_ = net_.start();
    snd_initialized_ = snd_.init();

    // Inicialización de TTS (en hilo para no bloquear)
    std::thread tLoadTTS([this]() {
            tts_initialized_ = tts_.init();
        }
    );
    tLoadTTS.detach();  // No necesitamos "esperar" a que termine
    
    // Hilo consumidor de paquetes Host
    worker_ = std::thread(&AppController::TWorker, this);

    return true;
}

int AppController::run() {
    running_ = true;
    gui_.run(); // Bloquea hasta cerrar
    return 0;
}


// Hilos --------------------------------------------------------------------------------
void AppController::TWorker() {
    SYS_INFO("TWorker","Initializating consumer thread...");
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
            SYS_WARN("TWorker", "'Host' Socket undefined");
            return; // <- TODO reiniciar o hacer algo para recuperarse de esto si pasa
        }

        data = net_.getDataFromSocket(index);

        if (data.empty()) {
            SYS_WARN("TWorker","Empty data received");
            continue;
        }

        // Procesar el paquete (simulado)
        SYS_INFO("TWorker", "Procesando paquete de datos...");
        SYS_INFO("TWorker","Size of data " + std::to_string(data.size()));
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }

    SYS_INFO("TWorker", "Consumer thread stopped.");
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

    
    // Sockets --------------------------------------------------------------------------
    
    bool AppController::addReceiver() const noexcept {
        // TODO
        return true;
    }

    bool AppController::removeReceiver() const noexcept {
        // TODO
        return true;
    }


    // Audio ----------------------------------------------------------------------------

    std::vector<std::string> AppController::getAvailableInputDevices() noexcept {
        return snd_.getAvailableInputs();
    }

    std::vector<std::string> AppController::getAvailablePlaybackDevices() noexcept {
        return snd_.getAvailablePlaybacks();
    }


    // TTS ----------------------------------------------------------------------------
    
    bool AppController::TTSgenerate(
        std::string const& modelName, 
        std::string const& text, 
        std::string const& wavname)
        noexcept {
            return tts_.generate(modelName, text, wavname);
        };

    short AppController::getTTSInitPercent() const noexcept {
        return tts_.getInitPercent();
    }

    std::vector<std::string> AppController::getAvailableTTSModels() noexcept {
        return tts_.getAvailableModels();
    };
      
    std::vector<std::string> AppController::getLoadedTTSModels() const noexcept {
        return tts_.getLoadedModels();
    };

    short AppController::getAvailableNumTTSModels() const noexcept {
        return tts_.getAvailableNumModels();
    }

    short AppController::getLoadedNumTTSModels() const noexcept {
        return tts_.getLoadedNumModels();
    }

    std::string AppController::getTTSProcessingText(std::string modelName) const noexcept {
        return tts_.getProccesingText(modelName);
    }

    