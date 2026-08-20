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
    #include <conio.h> // Para _getch()
    #include <windows.h>
#else
    #include <termios.h> 
    #include <unistd.h> 
#endif

// Función auxiliar de consola
void setConsoleTitle(const std::string& title) {
    #ifdef _WIN32
        SetConsoleTitleA(title.c_str());
    #else
        std::cout << "\033]0;" << title << "\007" << std::flush;
    #endif
}


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
    
    // Leer valores del json para AppController
    JsonMgr& jsonMgr = JsonMgr::instance();
    json* config = jsonMgr.load(config_filename_); // si no hay, nullptr
    // Almacenamiento temporal de nodos de json para cada módulo
    json* config_node = nullptr;

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);

    // Comprobar si se solicitó modo terminal explícito desde comandos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--cli" || arg == "--console" || "/c") {
            enable_gui_ = false;
            SYS_INFO("AppController", "CLI-only mode forced via command-line argument.");
            break;
        }
    }


    // Obtener el nombre del ejecutable e inicializar SystemMgr con ese nombre
    std::filesystem::path path = std::filesystem::absolute(argv[0]);
    app_name_ = path.stem().string();
    SystemMgr::instance().init(app_name_);

    // Fallback a uso de terminal si no hay GUI
    if (!enable_gui_) {
        launch_console();
        setConsoleTitle(app_name_);
    }

    SYS_INFO("AppController","Initializating application...");
    if (!config)
        SYS_WARN("AppController","Cannot load config. Using default values.");

    // Mostrar versión en log
    SYS_INFO("AppController","Welcome to " + std::string(app_name_) + ", version " + version_ + "!");


    // Iniciar GUI, salir si no se carga bien
    config_node = jsonMgr.getSubNode(config_filename_,"gui");
    if (enable_gui_) {
        SYS_INFO("AppController","GUI subsystem loading...");
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
    SYS_INFO("AppController","Running app...");
    if (enable_gui_ && gui_ && gui_->isInitialized())
        return gui_->run();         // Bloquea en la ventana en modo GUI
    else
        return run_cli_loop();      // Bloquea en la consola
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

bool AppController::run_cli_loop() {
    
    SYS_INFO("AppController", "Running in CLI Mode. Type 'help' for commands or 'exit' to quit.");

    std::string command;
    size_t      cursor = 0; // Posición del cursor dentro de "command" (no siempre al final)

    // Historial de comandos (flechas arriba/abajo)
    std::vector<std::string> history;
    size_t                   historyIndex = 0; // == history.size() cuando no se está navegando
    std::string              draft;            // Línea en curso, guardada al empezar a navegar el historial

    // Ocultar cursor en terminales ANSI para eliminar flicker visual
    //std::cerr << "\033[?25l";

    SystemMgr::instance().setCliActive(true);
    SystemMgr::instance().updateCliInput(command, cursor); // Pinta el primer prompt

    #ifndef _WIN32
        // LINUX: Apagar el "Cooked Mode" y el "Echo" automático de la terminal
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    #endif

    while (running_) {
        char c;

        #ifdef _WIN32
            // WINDOWS: Lee tecla sin imprimirla en pantalla. Las teclas especiales
            // (flechas, F1-F12...) llegan como DOS pulsaciones: un prefijo 0 o 224
            // seguido del código real (izq=75, der=77, arriba=72, abajo=80).
            int ch = _getch();

            // Comandos para las flechas y teclas especiales (no imprimibles)
            if (ch == 0 || ch == 224) {
                int code = _getch();
                if (code == 75 && cursor > 0) {                      // Flecha izquierda
                    cursor--;
                    SystemMgr::instance().updateCliInput(command, cursor);
                } else if (code == 77 && cursor < command.size()) {  // Flecha derecha
                    cursor++;
                    SystemMgr::instance().updateCliInput(command, cursor);
                } else if (code == 72) {                             // Flecha arriba: comando anterior
                    if (!history.empty() && historyIndex > 0) {
                        if (historyIndex == history.size())
                            draft = command;
                        command = history[--historyIndex];
                        cursor  = command.size();
                        SystemMgr::instance().updateCliInput(command, cursor);
                    }
                } else if (code == 80) {                             // Flecha abajo: comando siguiente
                    if (historyIndex < history.size()) {
                        ++historyIndex;
                        command = (historyIndex == history.size()) ? draft : history[historyIndex];
                        cursor  = command.size();
                        SystemMgr::instance().updateCliInput(command, cursor);
                    }
                }
                continue;
            }
            c = static_cast<char>(ch);
        #else
            // LINUX: Como hemos apagado el echo, no se imprime. Las flechas llegan
            // como secuencia de escape ESC [ A/B/C/D (izq=D, der=C, arriba=A, abajo=B).
            int ch = getchar();
            if (ch == 27) {
                if (getchar() == '[') {
                    int code = getchar();
                    if (code == 'D' && cursor > 0) {                     // Flecha izquierda
                        cursor--;
                        SystemMgr::instance().updateCliInput(command, cursor);
                    } else if (code == 'C' && cursor < command.size()) { // Flecha derecha
                        cursor++;
                        SystemMgr::instance().updateCliInput(command, cursor);
                    } else if (code == 'A') {                            // Flecha arriba: comando anterior
                        if (!history.empty() && historyIndex > 0) {
                            if (historyIndex == history.size())
                                draft = command;
                            command = history[--historyIndex];
                            cursor  = command.size();
                            SystemMgr::instance().updateCliInput(command, cursor);
                        }
                    } else if (code == 'B') {                            // Flecha abajo: comando siguiente
                        if (historyIndex < history.size()) {
                            ++historyIndex;
                            command = (historyIndex == history.size()) ? draft : history[historyIndex];
                            cursor  = command.size();
                            SystemMgr::instance().updateCliInput(command, cursor);
                        }
                    }
                }
                continue;
            }
            c = static_cast<char>(ch);
        #endif

        // Procesar pulsación
        if (c == '\n' || c == '\r') {
            // ENTER: Ejecutar comando
            SystemMgr::instance().setCliActive(false);
            std::cout << "\n"; // Forzar salto real

            execute_command(command);

            // Añadir al historial (evitando duplicar el mismo comando consecutivo)
            if (!command.empty() && (history.empty() || history.back() != command))
                history.push_back(command);
            historyIndex = history.size();
            draft.clear();

            command.clear();
            cursor = 0;
            SystemMgr::instance().setCliActive(true);
            SystemMgr::instance().updateCliInput(command, cursor); // Repinta el prompt vacío

        } else if (c == '\b' || c == 127) {
            // BACKSPACE (127 en la mayoría de UNIX): borra el carácter antes del cursor
            if (cursor > 0) {
                command.erase(cursor - 1, 1);
                cursor--;
                SystemMgr::instance().updateCliInput(command, cursor);
            }
        } else if (c >= 32 && c <= 126) {
            // CARÁCTER IMPRIMIBLE ESTÁNDAR: se inserta en la posición del cursor
            command.insert(command.begin() + cursor, c);
            cursor++;
            SystemMgr::instance().updateCliInput(command, cursor);
        }
    }

    // Restaurar visibilidad del cursor al salir
    //std::cerr << "\033[?25h" << std::flush;

    #ifndef _WIN32
        // LINUX: Restaurar la terminal a su estado original al salir
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    #endif

    return true;
}


namespace {

    // Divide una línea de comandos en tokens, respetando comillas dobles
    // ("MORSE 2" se convierte en un único token sin comillas: MORSE 2).
    std::vector<std::string> tokenize_cli(std::string const& line) {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;

        for (char c : line) {
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ' ' && !inQuotes) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty())
            tokens.push_back(current);

        return tokens;
    }

    // Extrae pares --flag valor de una lista de tokens (estilo rclone), a partir de "from".
    std::unordered_map<std::string, std::string> parse_flags(
        std::vector<std::string> const& tokens, size_t from)
    {
        std::unordered_map<std::string, std::string> flags;
        for (size_t i = from; i < tokens.size(); ++i) {
            if (tokens[i].rfind("--", 0) == 0) {
                std::string value = (i + 1 < tokens.size()) ? tokens[i + 1] : "";
                flags[tokens[i]] = value;
                ++i; // saltar el valor ya consumido
            }
        }
        return flags;
    }

} // namespace


void AppController::execute_command(const std::string& cmd) {

    std::vector<std::string> tokens = tokenize_cli(cmd);
    if (tokens.empty())
        return;

    std::string const& name = tokens[0];

    // Tabla de dispatch: nombre de comando -> handler(tokens).
    // Para añadir un comando nuevo basta con añadir una entrada aquí.
    using CliHandler = std::function<void(std::vector<std::string> const&)>;
    const std::unordered_map<std::string, CliHandler> handlers = {

        { "help", [](std::vector<std::string> const&) {
            std::cout << "Available commands:\n";
            std::cout << "  help                                 - Show this help message\n";
            std::cout << "  status                               - Show online/offline mode\n";
            std::cout << "  sounds help                          - Show sound-related commands\n";
            std::cout << "  exit, quit                           - Exit the application\n";
        }},

        { "status", [this](std::vector<std::string> const&) {
            SYS_INFO("CLI", "Online mode: " + std::string(isOnlineMode() ? "YES" : "NO"));
        }},

        { "sounds", [this](std::vector<std::string> const& t) {
            execute_sounds_command(t);
        }},

        { "exit", [this](std::vector<std::string> const&) {
            SYS_INFO("AppController", "Exiting application...");
            running_ = false;
        }},
        { "quit", [this](std::vector<std::string> const&) {
            SYS_INFO("AppController", "Exiting application...");
            running_ = false;
        }},
    };

    auto it = handlers.find(name);
    if (it != handlers.end())
        it->second(tokens);
    else
        SYS_WARN("CLI", "Unknown command: " + name);
}


void AppController::execute_sounds_command(std::vector<std::string> const& tokens) {

    // tokens[0] == "sounds"; tokens[1] es el subcomando
    std::string const sub = (tokens.size() > 1) ? tokens[1] : "help";

    if (sub == "help") {
        std::cout << "Sound commands:\n";
        std::cout << "  sounds morse \"<text>\" [--player \"<name>\"]  (default player: ADF)\n";
        return;
    }

    if (sub == "morse") {
        // Argumento posicional obligatorio: tokens[2] = texto a codificar
        if (tokens.size() < 3) {
            SYS_WARN("CLI", "Usage: sounds morse \"<text>\" [--player \"<name>\"]");
            return;
        }

        std::string const& text = tokens[2];

        auto flags = parse_flags(tokens, 3);
        auto playerIt = flags.find("--player");

        std::string playerName = "ADF";
        if (playerIt != flags.end()) {
            if (playerIt->second.empty())
                SYS_WARN("CLI", "--player requires a value, using default 'ADF'.");
            else
                playerName = playerIt->second;
        }

        PlayerMorse* pm = snd_->getPlayerMorse(playerName);
        if (!pm) {
            SYS_WARN("CLI", "Morse module '" + playerName + "' not available.");
            return;
        }

        pm->playMorse(text);
        return;
    }

    SYS_WARN("CLI", "Unknown sounds subcommand: " + sub);
}