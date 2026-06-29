#include <chrono>               // Controla tiempos de espera
#include "app/AppController.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

AppController::AppController() :
    argc_(0),
    argv_(nullptr),
    gui_(this),
    config_filename_("config.json"),
    net_initialized_(false),
    gui_initialized_(false),
    snd_initialized_(false),
    tts_initialized_(false),
    sym_initialized_(false),
    running_(false),
    online_mode_(true),
    version_("0.9")
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

    
    // Leer valores del json para AppController
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_);
    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else
        SYS_WARN("AppController","Cannot load config. Using default values.");


    // Almacenamiento temporal de nodos de json para cada módulo
    json* config_node = nullptr;    


    // Iniciar GUI, salir si no se carga bien
    SYS_INFO("AppController","GUI subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"gui");
    gui_initialized_ = gui_.init();
    if (!gui_initialized_) {
        SYS_ERROR("AppController","GUI subsystem FAIL");
        return false;   // Está diseñado para salir directamente si no hay GUI
    }
    else SYS_INFO("AppController","GUI subsystem OK");


    // Iniciar gestor de red
    SYS_INFO("AppController","Network subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"network");
    net_initialized_ = net_.init();
    if(!net_initialized_)
        SYS_ERROR("AppController","Network subsystem FAIL");
    else SYS_INFO("AppController","Network subsystem OK");


    // Iniciar gestor de sonidos
    SYS_INFO("AppController","Sound subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"sound");
    snd_initialized_ = snd_.init();
    if(!snd_initialized_)
        SYS_ERROR("AppController","Sound subsystem FAIL");
    else SYS_INFO("AppController","Sound subsystem OK");


    // Iniciar conexión Totalmix
    SYS_INFO("AppController","Totalmix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"totalmix");
    tmx_initialized_ = tmx_.init(config_node);
    if(!tmx_initialized_)
        SYS_WARN("AppController","Totalmix manager FAIL");
    else SYS_INFO("AppController","Totalmix manager OK");


    // Iniciar conexión Symetrix    
    SYS_INFO("AppController","Symetrix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"symetrix");
    sym_initialized_ = sym_.init(L"192.168.7.21");
    if(!sym_initialized_)
        SYS_WARN("AppController","Symetrix manager FAIL");
    else SYS_INFO("AppController","Symetrix manager OK");
    

    // Inicialización de TTS (en hilo para no bloquear)
    std::thread tLoadTTS([this]() {
            SYS_INFO("AppController","Starting TTS subsystem async load...");

            auto& jsonMgr = JsonMgr::instance();
            json* tts_config_node = jsonMgr.getSubNode(config_filename_, "tts");

            tts_initialized_ = tts_.init(tts_config_node);
            JsonMgr::instance().update(config_filename_);  
        }
    );
    tLoadTTS.detach();  // No necesitamos "esperar" a que termine


    // Volcar datos que hayan escrito los módulos al config
    jsonMgr.update(config_filename_);


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


// Configuración ------------------------------------------------------------------------

void AppController::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    jsonMgr.get_or_set(cfg, "version",  version_);

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

        // Salir si el programa se está cerrando (después de getpacket)
        if (!running_) break;

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


// IAppControl methods en IAppOverrides.cpp
