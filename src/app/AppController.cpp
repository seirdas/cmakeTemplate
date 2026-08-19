#include "app/AppController.hpp"
#include <string>               // Strings de texto
#include <chrono>               // Controla tiempos de espera
#include <filesystem>           // Controla directorios, rutas, etc.
#include <iostream>             // Consola

#include "gui/GuiMgr.hpp"       // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
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

// Soporte de consola en Linux
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h> // Para isatty() y STDIN_FILENO
#endif

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
    gui_(std::make_unique<GuiMgr>(this)),
    snd_(std::make_unique<SoundMgr>()),
    tmx_(std::make_unique<TotalMix>()),
    sym_(std::make_unique<Symetrix>()),
    vip_(std::make_unique<VoIPMgr>()),
    com_(std::make_unique<CommsCore>()),
    dds_(std::make_unique<FastDDS>()),
    cds_(std::make_unique<CycloneDDS>()),
    enable_net_(true),
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

    // Fallback a uso de terminal
    if (!enable_gui_) {
        launch_console();
    }

    SYS_INFO("AppController","Initializating application...");


    // Obtener el nombre del ejecutable e inicializar SystemMgr con ese nombre
    std::filesystem::path path = std::filesystem::absolute(argv[0]);
    app_name_ = path.stem().string();
    SystemMgr::instance().init(app_name_);


    // Leer valores del json para AppController
    SYS_INFO("AppController","Reading app config files...");
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_);
    // Almacenamiento temporal de nodos de json para cada módulo
    json* config_node = nullptr;


    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else
        SYS_WARN("AppController","Cannot load config. Using default values.");


    // Mostrar versión en log
    SYS_INFO("AppController","Welcome to " + std::string(app_name_) + ", version " + version_ + "!");


    // Iniciar GUI, salir si no se carga bien
    SYS_INFO("AppController","GUI subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"gui");
    if (enable_gui_) {
        if (!gui_->init(config_node))
            SYS_ERROR("AppController","GUI subsystem FAIL");
        else SYS_INFO("AppController","GUI subsystem OK");
    }

    // Comprobar si se ha iniciado GUI, si no, lanzar terminal
    if (enable_gui_ && !gui_->isInitialized()) {
        launch_console();
        SYS_INFO("AppController","Cannot initialize GUI . Fallback to terminal");
    }

    // Iniciar gestor de red
    SYS_INFO("AppController","Network subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"network");
    if (enable_net_) {
        if(!net_->init(config_node))
            SYS_ERROR("AppController","Network subsystem FAIL");
        else SYS_INFO("AppController","Network subsystem OK");
    }


    // Iniciar gestor de sonidos (+TTS)
    SYS_INFO("AppController","Sound subsystem loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"sound");
    if (enable_snd_) {
        if(!snd_->init(config_node))
            SYS_ERROR("AppController","Sound subsystem FAIL");
        else SYS_INFO("AppController","Sound subsystem OK");
    }


    // Iniciar conexión Totalmix
    SYS_INFO("AppController","Totalmix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"totalmix");
    if (enable_tmx_) {
        if(!tmx_->init(config_node))
            SYS_WARN("AppController","Totalmix manager FAIL");
        else SYS_INFO("AppController","Totalmix manager OK");
    }


    // Iniciar conexión Symetrix    
    SYS_INFO("AppController","Symetrix manager loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"symetrix");
    if (enable_sym_) {
        if(!sym_->init(config_node))
            SYS_WARN("AppController","Symetrix manager FAIL");
        else SYS_INFO("AppController","Symetrix manager OK");
    }

    
    // Inicialización lógica Comms
    SYS_INFO("AppController","Comms logic module loading...");
    config_node = jsonMgr.getSubNode(config_filename_,"comms");
    if (enable_com_) {
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

    // Hilo para cout de pruebas
    SYS_INFO("AppController","Starting test thread...");
    test_thread_ = std::thread(&AppController::TPruebas, this);


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

    jsonMgr.get_or_set(cfg, "version",  version_);
    jsonMgr.get_or_set(cfg, "app_enable_net_",  enable_net_);
    jsonMgr.get_or_set(cfg, "app_enable_gui_",  enable_gui_);
    jsonMgr.get_or_set(cfg, "app_enable_snd_",  enable_snd_);
    jsonMgr.get_or_set(cfg, "app_enable_tmx_",  enable_tmx_);
    jsonMgr.get_or_set(cfg, "app_enable_sym_",  enable_sym_);
    jsonMgr.get_or_set(cfg, "app_enable_voip_", enable_vip_);
    jsonMgr.get_or_set(cfg, "app_enable_com_",  enable_com_);
    jsonMgr.get_or_set(cfg, "app_enable_dds_",  enable_dds_);
    jsonMgr.get_or_set(cfg, "app_enable_cds_",  enable_cds_);

}

void AppController::close() {

    SYS_INFO("AppController","Closing AppController...");

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    /* Cerrar módulos (opcional, recomendado) */

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

    if (test_thread_.joinable()) {
        SYS_INFO("AppController","Waiting for test thread...");
        test_thread_.join();
    }


    SYS_INFO("AppController","AppController closed successfully");
}

bool AppController::run() {

    // Prueba simple de PlayerMorse
    snd_->addPlayerMorse(nullptr, "morse1", "");
    PlayerMorse* morse1 = snd_->getPlayerMorse("morse1");
    if (morse1)
        morse1->playMorse("AYUDA");


    // Prueba simple de PlayerTTS
    snd_->addPlayerTTS(nullptr, "pepe", "");
    PlayerTTS* pepe = snd_->getPlayerTTS("pepe");
    if (pepe != nullptr) {
        pepe->playTTS("en_US-amy-low", "ATIS information alfa connection, hello my name is Adri I like to fly and to play league of legends", "atis1");
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        pepe->stop("atis1", false, 0, 2000);
    }


    SYS_INFO("AppController","Running app...");
    if (enable_gui_ && gui_ && gui_->isInitialized())
        return gui_->run(); // Bloquea en la ventana ImGui
    else {
        run_cli_loop();       // Bloquea en la consola de comandos de texto
        return true;
    }
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

// IAppControl methods ------------------------------------------------------------------

/* IAppControl methods en IAppOverrides.cpp */


// Consola ------------------------------------------------------------------------------

bool AppController::launch_console() {

    SYS_INFO("AppController","Trying to launch console...");

    #ifdef _WIN32

        // Comprobar si ya hay una terminal
        if (GetConsoleWindow() != NULL) {
            SYS_INFO("AppController","Console already opened.");
            return false;
        }

        // Abrir terminal en Windows
        if (AllocConsole()) {
            FILE* dummy = nullptr;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONIN$", "r", stdin);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            
            // Desincronizar C y C++ stream buffers e inicializar estados
            std::ios::sync_with_stdio();
            std::cin.clear();
            std::cout.clear();
            std::cerr.clear();

            // Opcional: Activar procesamiento de colores ANSI en la consola de Windows 10/11
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode)) {
                    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }
            
            SYS_INFO("AppController", "Windows console allocated successfully.");
            return true;
        }

        return false;

    #else
        /* (WIP) */

        // Si la aplicación no tiene una TTY estándar asignada (lanzada por GUI/Doble clic)
        if (isatty(STDIN_FILENO)) {
            SYS_INFO("AppController","Console already opened.");
            return false;
        }

        SYS_INFO("AppController", "No TTY detected. Launching external terminal window...");
        std::string exe_path = argv_[0];

        // Lanzar terminal ejecutando binario dentro de ella
        std::string cmd = 
            "qterminal -e \"" + exe_path + "\" & || "
            "xfce4-terminal -e \"" + exe_path + "\" & || "
            "konsole -e \"" + exe_path + "\" & || "
            "alacritty -e \"" + exe_path + "\" & || "
            "xterm -e \"" + exe_path + "\" &";

        int ret = std::system(cmd.c_str());

        if (ret == 0) {
            // Al lanzar una instancia vinculada en la nueva ventana,
            // cerramos el proceso padre actual "ciego" que no tenía TTY.
            SYS_INFO("AppController", "Delegated execution to new terminal window. Exiting background process.");
            std::exit(0); 
        }

        SYS_ERROR("AppController", "Failed to launch external terminal.");
        return false;

    #endif
}

void AppController::run_cli_loop() {
    SYS_INFO("AppController", "Running in CLI Mode. Type 'help' for commands or 'exit' to quit.");
    std::string command;
    
    while (running_) {
        if (!(std::cin >> command)) break;

        if (command == "exit" || command == "quit") {
            close();
            break;
        } else if (command == "status") {
            std::cout << "Online mode: " << (isOnlineMode() ? "YES" : "NO") << "\n";
        } else if (command == "help") {
            std::cout << "Commands: status, exit\n";
        } else {
            std::cout << "Unknown command: " << command << "\n";
        }
    }
}
