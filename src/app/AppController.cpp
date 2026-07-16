#include "app/AppController.hpp"
#include <chrono>               // Controla tiempos de espera
#include <string>

#include "gui/GuiMgr.hpp"       // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "tts/TTSMgr.hpp"       // Clase para gestionar TTS
#include "devices/TotalMix.hpp" // Clase para gestionar driver TotalmixFX
#include "devices/Symetrix.hpp" // Clase para gestionar driver Symetrix Composer
#include "comms/CommsCore.hpp"  // Clase para lógica de comunicaciones
#include "voip/VoIPMgr.hpp"     // Clase para gestión Voiprec/Voipplay

#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

AppController::AppController() :
    running_(false),
    online_mode_(true),
    version_("0.0.0"),
    argc_(0),
    argv_(nullptr),
    config_filename_("config.json"),
    net_(std::make_unique<NetMgr>()),
    gui_(std::make_unique<GuiMgr>(this)),
    snd_(std::make_unique<SoundMgr>()),
    tts_(std::make_unique<TTSMgr>(snd_.get())),
    tmx_(std::make_unique<TotalMix>()),
    sym_(std::make_unique<Symetrix>()),
    voip_(std::make_unique<VoIPMgr>()),
    com_(std::make_unique<CommsCore>())
{

}

AppController::~AppController() {

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    // Cerrar socket y worker esperando paquetes de red
    net_->close();
    online_cv_.notify_all();
    SYS_INFO("AppController","Waiting for running threads...");
    if (hilo_consumer_.joinable())
        hilo_consumer_.join();

    if (hilo_test_.joinable())
        hilo_test_.join();

    // Cerrar módulos (opcional, recomendado)
    gui_->close();
    snd_->stop();
    tts_->close();
    //tmx_->close();       // #TODO
    sym_->close();

    SYS_INFO("AppController","AppController closed successfuly.");
}

bool AppController::init(int argc, char** argv) {

    SYS_INFO("AppController","Initializating application...");

    // Obtiene los parámetros de entrada
    this->argc_ = argc;
    this->argv_ = argv;

    
    // Leer valores del json para AppController
    SYS_INFO("AppController","Reading app config files...");
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_);
    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else
        SYS_WARN("AppController","Cannot load config. Using default values.");


    // Almacenamiento temporal de nodos de json para cada módulo
    json* config_node = nullptr;


    // Mostrar versión en log
    SYS_INFO("AppController","Welcome to " + std::string(APP_NAME) + ", version " + version_ + "!");


    // Iniciar GUI, salir si no se carga bien
    SYS_INFO("AppController","GUI subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"gui");
    if (!gui_->init(config_node)) {
        SYS_ERROR("AppController","GUI subsystem FAIL");
        return false;   // Está diseñado para salir directamente si no hay GUI
    }
    else SYS_INFO("AppController","GUI subsystem OK");


    // Iniciar gestor de red
    SYS_INFO("AppController","Network subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"network");
    if(!net_->init(config_node))
        SYS_ERROR("AppController","Network subsystem FAIL");
    else SYS_INFO("AppController","Network subsystem OK");


    // Iniciar gestor de sonidos
    SYS_INFO("AppController","Sound subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"sound");
    if(!snd_->init(config_node))
        SYS_ERROR("AppController","Sound subsystem FAIL");
    else SYS_INFO("AppController","Sound subsystem OK");


    // Iniciar conexión Totalmix
    SYS_INFO("AppController","Totalmix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"totalmix");
    if(!tmx_->init(config_node))
        SYS_WARN("AppController","Totalmix manager FAIL");
    else SYS_INFO("AppController","Totalmix manager OK");


    // Iniciar conexión Symetrix    
    SYS_INFO("AppController","Symetrix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"symetrix");
    if(!sym_->init(config_node))
        SYS_WARN("AppController","Symetrix manager FAIL");
    else SYS_INFO("AppController","Symetrix manager OK");

    
    // Inicialización de TTS
    SYS_INFO("AppController","TTS manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"tts");
    if(!tts_->init(config_node))
        SYS_WARN("AppController","TTS manager FAIL");
    else SYS_INFO("AppController","TTS manager OK");

    
    // Inicialización lógica Comms
    SYS_INFO("AppController","Comms logic loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"comms");
    if(!com_->init(config_node))
        SYS_WARN("AppController","Comms logic FAIL");
    else SYS_INFO("AppController","Comms logic OK");
    
    // Activar running para los hilos
    running_ = true;

    // Hilo consumidor de paquetes online
    SYS_INFO("AppController","Starting net consumer thread...");
    hilo_consumer_ = std::thread(&AppController::TWorker, this);


    // Hilo para cout de pruebas
    SYS_INFO("AppController","Starting test thread...");
    hilo_test_ = std::thread(&AppController::TPruebas, this);


    // Volcar datos que hayan escrito los módulos al config
    SYS_INFO("AppController","Updating json config files...");
    jsonMgr.update();


    SYS_INFO("AppController","App initialized.");
    return true;
}

int AppController::run() {
    SYS_INFO("AppController","Running app...");
    gui_->run(); // ← Bloquea hasta cerrar
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
        data = net_->getNextUdpPacket();         // <- BLOQUEANTE

        // Salir si el programa se está cerrando (después de getpacket)
        if (!running_) break;

        // Si se ha puesto en modo offline, continuar (para bloquear en espera)
        if (!online_mode_) 
            continue;

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
    // Poner aquí las pruebas o lo que sea
    /*
    while (running_) {
        SYS_INFO("TPruebas","Udp queue size: " + std::to_string(net_.numUdpRcvElements()));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    */
}


// IAppControl methods en IAppOverrides.cpp
