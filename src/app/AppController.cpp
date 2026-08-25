#include "app/AppController.hpp"
#include <string>               // Strings de texto
#include <chrono>               // Controla tiempos de espera
#include <filesystem>           // Controla directorios, rutas, etc.

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


// General ------------------------------------------------------------------------------

AppController::AppController(int argc, char** argv) :
    version_("0.0.0"),
    initialized_(false),
    running_(false),
    online_mode_(true),
    argc_(argc),
    argv_(argv),
    config_filename_("config.json"),
    net_(std::make_unique<NetMgr>()),
    cli_(std::make_unique<ConsoleMgr>()),
    gui_(std::make_unique<GuiMgr>()),
    snd_(std::make_unique<SoundMgr>()),
    tmx_(std::make_unique<TotalMix>()),
    sym_(std::make_unique<Symetrix>()),
    vip_(std::make_unique<VoIPMgr>()),
    com_(std::make_unique<CommsCore>()),
    dds_(std::make_unique<FastDDS>()),
    cds_(std::make_unique<CycloneDDS>())
{
    // Establecer controladores de las interfaces de usuario
    gui_->setController(this);
    cli_->setController(this);
    cli_->setAppName(argv_[0]);

    // Activa todos los módulos por defecto
    enable_flags_.setAll(1);
}

AppController::~AppController() {
    close();
}


// Inicialización y ejecución -----------------------------------------------------------

bool AppController::init() {
    
    // Leer valores del json para AppController
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_); // si no hay, nullptr

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);

    // Comprobar si se solicitó modo terminal explícito desde comandos
    for (int token = 1; token < argc_; ++token) {
        std::string arg = argv_[token];
        if (arg == "-c" || arg == "--cli" || arg == "--console" || arg == "/c") {
            enable_flags_.gui = false;
            enable_flags_.cli = true;
            SYS_INFO("AppController", "CLI-only mode forced via command-line argument.");
            break;
        }
    }

    // Obtener el nombre del ejecutable e inicializar SystemMgr con ese nombre
    std::filesystem::path path = std::filesystem::absolute(argv_[0]);
    app_name_ = path.stem().string();
    SystemMgr::instance().init(app_name_);

    // Inicializar módulos de interfaz
    init_module(gui_, "GUI", enable_flags_.gui);
    init_module(cli_, "CLI", enable_flags_.cli);

    // Si la gui no está, lanzar ya la terminal de cli (para que se vean los logs del init)
    if(!gui_->isInitialized() && cli_->isInitialized())
        cli_->LaunchConsole();

    // Info
    SYS_INFO("AppController","Initializating application...");
    if (!config)
        SYS_WARN("AppController","Cannot load config. Using default values.");

    // Mostrar versión en log
    SYS_INFO("AppController","Welcome to " + std::string(app_name_) + ", version " + version_ + "!");


    // Iniciar módulos
    init_module(net_, "Network",            enable_flags_.net);
    init_module(snd_, "Sounds",             enable_flags_.snd);
    init_module(tmx_, "Totalmix",           enable_flags_.tmx);
    init_module(sym_, "Symetrix",           enable_flags_.sym);
    init_module(com_, "CommsDispatcher",    enable_flags_.com);
    init_module(dds_, "FastDDS",            enable_flags_.dds);
    init_module(cds_, "CycloneDDS",         enable_flags_.cds);


    // Vincular módulos a través de patrón observador a GUI ( #TODO )
    // if (gui_->isInitialized() && tts_->isInitialized())
    //     tts_->addObserver(gui_.get());


    // Volcar datos que hayan escrito los módulos al config
    SYS_INFO("AppController","Updating json config files...");
    jsonMgr.update();

    
    // Informar y salir
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
    
    // Para los bitfields se necesita un bool:
    bool val = false;
    
    val = enable_flags_.net;
    jsonMgr.get_or_set(cfg, "APP_enable_network", val);
    enable_flags_.net = val;

    val = enable_flags_.cli;
    jsonMgr.get_or_set(cfg, "APP_enable_cli", val);
    enable_flags_.cli = val;

    val = enable_flags_.gui;
    jsonMgr.get_or_set(cfg, "APP_enable_gui", val);
    enable_flags_.gui = val;

    val = enable_flags_.snd;
    jsonMgr.get_or_set(cfg, "APP_enable_sounds", val);
    enable_flags_.snd = val;

    val = enable_flags_.tmx;
    jsonMgr.get_or_set(cfg, "APP_enable_totalmix", val);
    enable_flags_.tmx = val;

    val = enable_flags_.sym;
    jsonMgr.get_or_set(cfg, "APP_enable_symetrix", val);
    enable_flags_.sym = val;

    val = enable_flags_.vip;
    jsonMgr.get_or_set(cfg, "APP_enable_voip", val);
    enable_flags_.vip = val;

    val = enable_flags_.com;
    jsonMgr.get_or_set(cfg, "APP_enable_comms", val);
    enable_flags_.com = val;

    val = enable_flags_.dds;
    jsonMgr.get_or_set(cfg, "APP_enable_fastdds", val);
    enable_flags_.dds = val;

    val = enable_flags_.cds;
    jsonMgr.get_or_set(cfg, "APP_enable_cycloneds", val);
    enable_flags_.cds = val;

}

void AppController::close() {

    SYS_INFO("AppController","Closing AppController...");

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    // Cerrar módulos (opcional, recomendado)
    close_module(cli_, "CLI");
    close_module(gui_, "GUI");
    close_module(net_, "Network");
    close_module(snd_, "Sound");
    close_module(tmx_, "TotalMix");
    close_module(sym_, "Symetrix");
    close_module(com_, "Comms");
    close_module(dds_, "FastDDS");
    close_module(cds_, "CycloneDDS");

    SYS_INFO("AppController","AppController closed successfully");
}

bool AppController::run() {
    SYS_INFO("AppController","Running app...");
    if (enable_flags_.gui && gui_ && gui_->isInitialized())
        return gui_->Run();     // Bloquea en la ventana en modo GUI
    else if (enable_flags_.cli && cli_ && cli_->isInitialized())
        return cli_->Run();      // Bloquea en la consola
    else
        SYS_ERROR("AppController","Cannot run: no user interface enabled (GUI/CLI)");

    // Llega aquí si no ha entrado en ningún bucle
    return false;
}


// IAppControl methods ------------------------------------------------------------------

    // Aplicación -----------------------------------------------------------------------

    std::string AppController::getVersion() const noexcept { 
        return version_; 
    }

    void AppController::setOnlineMode(bool nuevo_online_mode) noexcept { 
        
        // Comprobar si el modo ha cambiado
        if(online_mode_==nuevo_online_mode) 
            return;
        
        // Establecer modo
        online_mode_=nuevo_online_mode;
        SYS_INFO("IAppControl", std::string(online_mode_ ? "ONLINE" : "OFFLINE") + " mode set.");

        // Activar/desactivar la red
        if(online_mode_)
            net_->start();
        else
            net_->stop();
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


// Módulos ------------------------------------------------------------------------------

template <typename T>
void AppController::init_module(T& module, std::string const& name, bool enabled) {
    
    // Comprobar si el módulo está activado
    if (!enabled) {
        SYS_INFO("AppController", name + " subsystem disabled");
        return;
    }

    // Obtener la intancia de json y la configuación del módulo
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config_node = jsonMgr.getSubNode(config_filename_, name);

    // Inicializar con su configuración
    SYS_INFO("AppController", name + " loading...");
    if (module->init(config_node))      // CUIDADO: aqui supone que todos los módulos tienen init(config)
        SYS_INFO("AppController", name + " OK");
    else
        SYS_WARN("AppController", name + " FAIL");
}

template <typename T>
void AppController::close_module(T& module, std::string const& name) {
    if (module->isInitialized()) {
        SYS_INFO("AppController","Closing " + name + " subsystem...");
        module->close();
    }
}
