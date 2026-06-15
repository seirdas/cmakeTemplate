#include "app/AppController.hpp"
#include "system/SystemMgr.hpp"
#include <chrono>               // Controla tiempos de espera

// General ------------------------------------------------------------------------------

AppController::AppController() :
    argc_(0),
    argv_(nullptr),
    gui_(this),
    net_initialized_(false),
    gui_initialized_(false),
    snd_initialized_(false),
    tts_initialized_(false),
    sym_initialized_(false),
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
    if (hilo_consumer_.joinable())
        hilo_consumer_.join();

    if (hilo_test_.joinable())
        hilo_test_.join();

    // Cerrar módulos (opcional, recomendado)
    snd_.stop();
    gui_.cerrar();
    tts_.cerrar();

    SYS_INFO("AppController","AppController closed successfuly.");
}

bool AppController::init(int argc, char** argv) {

    SYS_INFO("AppController","Initializating application...");
    
    // Obtiene los parámetros de entrada
    this->argc_ = argc;
    this->argv_ = argv;

    // Iniciar GUI, salir si no se carga bien
    SYS_INFO("AppController","GUI subsystem loading...");
    gui_initialized_ = gui_.init();
    if (!gui_initialized_) {
        SYS_ERROR("AppController","GUI subsystem FAIL");
        return false;   // Está diseñado para salir directamente si no hay GUI
    }
    else SYS_INFO("AppController","GUI subsystem OK");


    // Iniciar gestor de red
    SYS_INFO("AppController","Network subsystem loading...");
    net_initialized_ = net_.start();
    if(!net_initialized_)
        SYS_ERROR("AppController","Network subsystem FAIL");
    else SYS_INFO("AppController","Network subsystem OK");


    // Iniciar gestor de sonidos
    SYS_INFO("AppController","Sound subsystem loading...");
    snd_initialized_ = snd_.init();
    if(!snd_initialized_)
        SYS_ERROR("AppController","Sound subsystem FAIL");
    else SYS_INFO("AppController","Sound subsystem OK");


    // Iniciar conexión Totalmix
    SYS_INFO("AppController","Totalmix manager loading...");
    tmx_initialized_ = tmx_.init(12321,"127.0.0.1", 7001,"127.0.0.1",32,32,32);
    if(!tmx_initialized_)
        SYS_WARN("AppController","Totalmix manager FAIL");
    else SYS_INFO("AppController","Totalmix manager OK");


    // Iniciar conexión Symetrix
    SYS_INFO("AppController","Symetrix manager loading...");
    sym_initialized_ = sym_.Init(L"192.168.7.21");
    if(!sym_initialized_)
        SYS_WARN("AppController","Symetrix manager FAIL");
    else SYS_INFO("AppController","Symetrix manager OK");
    

    // Inicialización de TTS (en hilo para no bloquear)
    std::thread tLoadTTS([this]() {
            SYS_INFO("AppController","Starting TTS subsystem async load...");
            tts_initialized_ = tts_.init();
        }
    );
    tLoadTTS.detach();  // No necesitamos "esperar" a que termine
    

    SYS_INFO("AppController","App initialized.");
    return true;
}

int AppController::run() {
    SYS_INFO("AppController","Running app...");
    running_ = true;
    
    // Hilo consumidor de paquetes online
    hilo_consumer_ = std::thread(&AppController::TWorker, this);

    // Hilo para cout de pruebas
    hilo_test_ = std::thread(&AppController::TPruebas, this);

    gui_.run(); // ← Bloquea hasta cerrar
    return 0;
}


// Hilos --------------------------------------------------------------------------------

void AppController::TWorker() {
    SYS_INFO("TWorker","Initializating consumer thread...");
    std::vector<char> data;

    while (running_) {

        // Forzar la espera hasta que sea notificado de un paquete nuevo
        std::unique_lock<std::mutex> lock(online_mtx_);
        online_cv_.wait(lock, [this] {
            return !running_ || online_mode_;
        });

        // Salir si el programa se está cerrando
        if (!running_) break;

        // Libera el lock de aquí en adelante
        lock.unlock();

        // Pide datos al socket para procesar
        data = net_.getNextUdpPacket();

        // Si no hay datos no hacer nada
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


void AppController::TPruebas() {
    SYS_INFO("TPruebas","init TPruebas");

    // Poner aquí las pruebas o lo que sea
    /*
    while (running_) {
        SYS_INFO("TPruebas","Udp queue size: " + std::to_string(net_.numUdpRcvElements()));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    */

    SYS_INFO("TPruebas","fin TPruebas");
}


// IAppControl methods ------------------------------------------------------------
    
    /* 
    * Aquí solo deberían ir acciones que se ejecuten sobre otros módulos
    * Para la "solicitud" de datos, se debería hacer un patrón observador
    */

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
        return false;
    }

    bool AppController::removeReceiver() const noexcept {
        // TODO
        return false;
    }


    // Audio ----------------------------------------------------------------------------

    std::vector<std::string> AppController::getAvailableInputDevices() noexcept {
        return snd_.getAvailableInputs();
    }

    std::vector<std::string> AppController::getAvailablePlaybackDevices() noexcept {
        return snd_.getAvailablePlaybacks();
    }

    void AppController::refreshAudioDevices() noexcept {
        snd_.updateDevices();
        return;
    }

    bool AppController::addInputDevice(std::string const& name) noexcept {
        return snd_.addCaptureDevice(name, 0);
    }

    bool AppController::removeInputDevice(unsigned short index) noexcept {
        return snd_.removeInputDevice(index);
    }

    bool AppController::StartRecording(unsigned short index) noexcept{
        return snd_.startRec_snd(index); 
    }
    
    bool AppController::StopRecording(unsigned short index) noexcept{
        return snd_.stopRec_snd(index); 
    }

    size_t AppController::getInputBufferSize(unsigned short index) noexcept {
        return snd_.getInputBufferSize(index); 
    }

    size_t AppController::getInputRecBufferSize(unsigned short index) noexcept {
        return snd_.getInputRecBufferSize(index); 
    }

    bool AppController::isInputDeviceValid(unsigned short index) noexcept {
        return snd_.isInputDeviceValid(index);
    }




    // TTS ----------------------------------------------------------------------------
    
    bool AppController::TTSgenerate(
        std::string const& modelName, 
        std::string const& text, 
        std::string const& wavname)
        noexcept {
            return tts_.generate(modelName, text, wavname);
        };

    TTSData AppController::getTTSData() noexcept {
        TTSData data;

        data.available_models     = tts_.getAvailableModels();
        data.loaded_models        = tts_.getLoadedModels();
        data.num_available_models = tts_.numAvailableModels();
        data.num_loaded_models    = tts_.numLoadedModels();
        data.init_percent         = static_cast<short>(100.0 * data.num_loaded_models / data.num_available_models);
        
        return data;
    }

    std::string AppController::getTTSProcessingText(std::string modelName) const noexcept {
        return tts_.getProccesingText(modelName);
    }
