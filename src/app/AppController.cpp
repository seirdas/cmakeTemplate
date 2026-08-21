#include "app/AppController.hpp"
#include <string>               // Strings de texto
#include <chrono>               // Controla tiempos de espera
#include <filesystem>           // Controla directorios, rutas, etc.
#include <iostream>             // Consola
#include <stdexcept>            // std::invalid_argument / std::out_of_range (parseo de comandos CLI)

#include "gui/GuiMgr.hpp"       // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "cli/ConsoleMgr.hpp"   // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "devices/TotalMix.hpp" // Clase para gestionar driver TotalmixFX
#include "devices/Symetrix.hpp" // Clase para gestionar driver Symetrix Composer
#include "logic/comms/CommsCore.hpp"  // Clase para lógica de comunicaciones
#include "voip/VoIPMgr.hpp"     // Clase para gestión Voiprec/Voipplay
#include "dds/FastDDS.hpp"      // Clase para gestión de DDS (con FastDDS)
#include "dds/CycloneDDS.hpp"   // Clase para gestión de DDS (con CycloneDDS)
#include "files/JsonMgr.hpp"    // Gestión de archivos json
#include "system/SystemMgr.hpp" // Gestión de log del sistema
#include "sound/PlayerMorse.hpp"
#include "sound/PlayerTTS.hpp"


// Funciones de consola (pendiente de meter en una clase aparte)
namespace {

} // namespace


// General ------------------------------------------------------------------------------

AppController::AppController() :
    version_("0.0.0"),
    initialized_(false),
    running_(false),
    online_mode_(true),
    argc_(0),
    argv_(nullptr),
    config_filename_("config.json"),
    net_(std::make_unique<NetMgr>()),
    cli_(std::make_unique<ConsoleMgr>(this)),
    gui_(std::make_unique<GuiMgr>(this)),
    snd_(std::make_unique<SoundMgr>()),
    tmx_(std::make_unique<TotalMix>()),
    sym_(std::make_unique<Symetrix>()),
    vip_(std::make_unique<VoIPMgr>()),
    com_(std::make_unique<CommsCore>()),
    dds_(std::make_unique<FastDDS>()),
    cds_(std::make_unique<CycloneDDS>()),
    enable_net_(true),
    enable_cli_(true),
    enable_gui_(true),
    enable_snd_(true),
    enable_tmx_(true),
    enable_sym_(true),
    enable_vip_(true),
    enable_com_(true),
    enable_dds_(true),
    enable_cds_(true)
{

}

AppController::~AppController() {
    close();
}


// Inicialización y ejecución -----------------------------------------------------------

bool AppController::init(int argc, char** argv) {

    // Obtiene los parámetros de entrada
    this->argc_ = argc;
    this->argv_ = argv;
    
    // Leer valores del json para AppController
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_); // si no hay, nullptr
    // Almacenamiento temporal de nodos de json para cada módulo
    json* config_node = nullptr;

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);

    // Comprobar si se solicitó modo terminal explícito desde comandos
    for (int token = 1; token < argc; ++token) {
        std::string arg = argv[token];
        if (arg == "-c" || arg == "--cli" || arg == "--console" || "/c") {
            enable_gui_ = false;
            enable_cli_ = true;
            SYS_INFO("AppController", "CLI-only mode forced via command-line argument.");
            break;
        }
    }

    // Obtener el nombre del ejecutable e inicializar SystemMgr con ese nombre
    std::filesystem::path path = std::filesystem::absolute(argv[0]);
    app_name_ = path.stem().string();
    SystemMgr::instance().init(app_name_);

    // Fallback a uso de terminal si no hay GUI
    if (!enable_gui_ && enable_cli_) {
        
        config_node = jsonMgr.getSubNode(config_filename_,"cli");
        SYS_INFO("AppController","CLI subsystem loading...");
        if (!cli_->init(config_node))
            SYS_ERROR("AppController","CLI subsystem FAIL");
        else SYS_INFO("AppController","CLI subsystem OK");

        // Abrir la consola
        if(cli_->isInitialized()) {
            cli_->LaunchConsole();
            cli_->setConsoleTitle(app_name_);
        }
    }

    SYS_INFO("AppController","Initializating application...");
    if (!config)
        SYS_WARN("AppController","Cannot load config. Using default values.");

    // Mostrar versión en log
    SYS_INFO("AppController","Welcome to " + std::string(app_name_) + ", version " + version_ + "!");


    // Iniciar GUI, salir si no se carga bien
    if (enable_gui_) {
        config_node = jsonMgr.getSubNode(config_filename_,"gui");
        SYS_INFO("AppController","GUI subsystem loading...");
        if (!gui_->init(config_node))
            SYS_ERROR("AppController","GUI subsystem FAIL");
        else SYS_INFO("AppController","GUI subsystem OK");
    }

    // Comprobar si se ha iniciado GUI, si no, lanzar terminal
    if (enable_gui_ && !gui_->isInitialized()) {
        cli_->LaunchConsole();
        SYS_INFO("AppController","Cannot initialize GUI . Fallback to terminal");
    }


    // Iniciar gestor de red
    if (enable_net_) {
        SYS_INFO("AppController","Network subsystem loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"network");
        if(!net_->init(config_node))
            SYS_ERROR("AppController","Network subsystem FAIL");
        else SYS_INFO("AppController","Network subsystem OK");
    }


    // Iniciar gestor de sonidos (+TTS)
    if (enable_snd_) {
        SYS_INFO("AppController","Sound subsystem loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"sound");
        if(!snd_->init(config_node))
            SYS_ERROR("AppController","Sound subsystem FAIL");
        else SYS_INFO("AppController","Sound subsystem OK");
    }


    // Iniciar conexión Totalmix
    if (enable_tmx_) {
        SYS_INFO("AppController","Totalmix manager loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"totalmix");
        if(!tmx_->init(config_node))
            SYS_WARN("AppController","Totalmix manager FAIL");
        else SYS_INFO("AppController","Totalmix manager OK");
    }


    // Iniciar conexión Symetrix    
    if (enable_sym_) {
        SYS_INFO("AppController","Symetrix manager loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"symetrix");
        if(!sym_->init(config_node))
            SYS_WARN("AppController","Symetrix manager FAIL");
        else SYS_INFO("AppController","Symetrix manager OK");
    }

    
    // Inicialización lógica Comms
    if (enable_com_) {
        SYS_INFO("AppController","Comms logic module loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"comms");
        if(!com_->init(config_node))
            SYS_WARN("AppController","Comms logic module FAIL");
        else SYS_INFO("AppController","Comms logic module OK");
    }

    
    // Inicialización FastDDS
    if (enable_dds_) {
        SYS_INFO("AppController","FastDDS manager loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"fastdds");
        if(!dds_->init(config_node))
            SYS_WARN("AppController","FastDDS manager FAIL");
        else SYS_INFO("AppController","FastDDS manager OK");
    }

    
    // Inicialización CycloneDDS
    if (enable_cds_) {
        SYS_INFO("AppController","CycloneDDS manager loading...");
        config_node = jsonMgr.getSubNode(config_filename_,"cyclonedds");
        if(!cds_->init(config_node))
            SYS_WARN("AppController","CycloneDDS manager FAIL");
        else SYS_INFO("AppController","CycloneDDS manager OK");
    }


    // Vincular módulos a través de patrón observador a GUI ( #TODO )
    // if (gui_->isInitialized() && tts_->isInitialized())
    //     tts_->addObserver(gui_.get());
    

    // Activar running para los hilos
    running_ = true;

    // Hilo consumidor de paquetes online
    SYS_INFO("AppController","Starting net consumer thread...");
    consumer_thread_ = std::thread(&AppController::TWorker, this);

    // Volcar datos que hayan escrito los módulos al config
    SYS_INFO("AppController","Updating json config files...");
    jsonMgr.update();


    SYS_INFO("AppController","App initialized");
    return true;
}

bool AppController::isInitialized() const {
    return initialized_;
}

void AppController::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    jsonMgr.get_or_set(cfg, "version",                  version_);
    jsonMgr.get_or_set(cfg, "app_enable_network",       enable_net_);
    jsonMgr.get_or_set(cfg, "app_enable_gui",           enable_gui_);
    jsonMgr.get_or_set(cfg, "app_enable_sounds",        enable_snd_);
    jsonMgr.get_or_set(cfg, "app_enable_totalmix",      enable_tmx_);
    jsonMgr.get_or_set(cfg, "app_enable_symetrix",      enable_sym_);
    jsonMgr.get_or_set(cfg, "app_enable_voip",          enable_vip_);
    jsonMgr.get_or_set(cfg, "app_enable_comms",         enable_com_);
    jsonMgr.get_or_set(cfg, "app_enable_fastdds",       enable_dds_);
    jsonMgr.get_or_set(cfg, "app_enable_cycloneds",     enable_cds_);

}

void AppController::close() {

    SYS_INFO("AppController","Closing AppController...");

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    /* Cerrar módulos (opcional, recomendado) */
    
    if (cli_->isInitialized()) {
        SYS_INFO("AppController","Closing GUI subsystem...");
        gui_->close();
    }

    if (gui_->isInitialized()) {
        SYS_INFO("AppController","Closing GUI subsystem...");
        gui_->close();
    }

    if (net_->isInitialized()) { // IMPRESCINDIBLE PARA CERRAR HILOS CONSUMIDORES
        SYS_INFO("AppController","Closing Network subsystem...");
        net_->close();
    }

    if (snd_->isInitialized()) {
        SYS_INFO("AppController","Closing Sound subsystem...");
        snd_->close();
    }
    
    if (tmx_->isInitialized()) {
        SYS_INFO("AppController","Closing Totalmix subsystem...");
        tmx_->close();
    }

    if (sym_->isInitialized()) {
        SYS_INFO("AppController","Closing Symetrix manager...");
        sym_->close();
    }

    if (com_->isInitialized()) {
        SYS_INFO("AppController","Closing Comms logic module...");
        com_->close();
    }

    if (dds_->isInitialized()) {
        SYS_INFO("AppController","Closing FastDDS manager...");
        dds_->close();
    }

    if (cds_->isInitialized()) {
        SYS_INFO("AppController","Closing CycloneDDS manager...");
        cds_->close();
    }
    
    // Cerrar hilos pendientes de aplicación
    online_cv_.notify_all();
    if (consumer_thread_.joinable()) {
        SYS_INFO("AppController","Waiting for consumer thread...");
        consumer_thread_.join();
    }

    SYS_INFO("AppController","AppController closed successfully");
}

bool AppController::run() {
    SYS_INFO("AppController","Running app...");
    if (enable_gui_ && gui_ && gui_->isInitialized())
        return gui_->Run();     // Bloquea en la ventana en modo GUI
    else if (enable_cli_)
        return cli_->Run();      // Bloquea en la consola
    else
        SYS_ERROR("AppController","Cannot run: no user interface enabled (GUI/CLI)");

    // Llega aquí si no ha entrado en ningún bucle
    return false;
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


// IAppControl methods ------------------------------------------------------------------

    // Aplicación -----------------------------------------------------------------------

    std::string AppController::getVersion() const noexcept { 
        return version_; 
    }

    void AppController::setOnlineMode(bool nuevo_online_mode) noexcept { 

        std::unique_lock<std::mutex> lock(online_mtx_);
        
        if(online_mode_==nuevo_online_mode) 
            return;
        
        online_mode_=nuevo_online_mode;
        lock.unlock();

        SYS_INFO("IAppControl", std::string(online_mode_ ? "ONLINE" : "OFFLINE") + " mode set.");

        if(online_mode_) {
            net_->start();
            online_cv_.notify_all();
        }
        else {
            net_->stop();
            online_cv_.notify_all();
        }
    };

    bool AppController::isOnlineMode() const noexcept {
        return online_mode_;
    };


    // Módulos --------------------------------------------------------------------------

    NetMgr* AppController::getNetModule() {
        return net_.get();
    }

    SoundMgr* AppController::getSoundsModule() {
        return snd_.get();
    }

    TotalMix* AppController::getTotalmixModule() {
        return tmx_.get();
    }

    Symetrix* AppController::getSymetrixModule() {
        return sym_.get();
    }
