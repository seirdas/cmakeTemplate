#include "cli/ConsoleMgr.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "system/ANSI.hpp"

// Módulos
#include "app/IAppControl.hpp"
#include "sound/SoundMgr.hpp"
#include "sound/PlayerAudio.hpp"
#include "sound/PlayerMorse.hpp"
#include "sound/PlayerTTS.hpp"
#include "devices/TotalMix.hpp"
#include "devices/Symetrix.hpp"
#include "net/NetMgr.hpp"

// Otros
#include <iostream>
#include <functional>


// General ------------------------------------------------------------------------------

ConsoleMgr::ConsoleMgr(IAppControl* ctrl, std::string const& exePath) :
    ctrl_(ctrl),
    initialized_(false),
    running_(false),
    AppName_("app"),
    exe_path_(exePath),
    tried_to_launch_console_(false)
{

}

ConsoleMgr::~ConsoleMgr() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool ConsoleMgr::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("ConsoleMgr","Cannot load config. Using default values.");

    // Validar ctrl
    if(!ctrl_)
        SYS_ERROR("ConsoleMgr","Cannot bind to app modules (ctrl=null)");
    
    initialized_ = true;
    return initialized_;    //<- true
}

bool ConsoleMgr::isInitialized() const {
    return initialized_;
}

void ConsoleMgr::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    jsonMgr.get_or_set(cfg, "app_name_window",	AppName_);

}

bool ConsoleMgr::close() {

    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;

    initialized_ = false;
    return !initialized_;   // <- true
}


// Ejecución ----------------------------------------------------------------------------

bool ConsoleMgr::Run() {

    // Inicializar consola
    LaunchConsole();

    // Cambiar el título de la consola
    setConsoleTitle(AppName_);
    
    // Info
    SYS_INFO("ConsoleMgr", "Running in CLI Mode. Type 'help' for commands or 'exit' to quit.");
    running_ = true;

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
            command.insert(
                command.begin() +
                static_cast<std::string::difference_type>(cursor),
                c
            );
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


// Opciones de consola ------------------------------------------------------------------

bool ConsoleMgr::LaunchConsole(bool force) {

    // Si ya lo ha intentado, no hacerlo de nuevo (evita errores duplicados)
    if(tried_to_launch_console_ && !force)
        return true;
    tried_to_launch_console_ = true;

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

            // Opcional: Activar procesamiento de colores ANSI en la consola de Windows 10/11
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode)) {
                    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }
            
            std::cout << std::endl;
            SYS_INFO("AppController", "Console allocated successfully");
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

        // Lanzar terminal ejecutando binario dentro de ella
        std::string cmd = 
            "qterminal -e \"" + exe_path_ + "\" & || "
            "xfce4-terminal -e \"" + exe_path_ + "\" & || "
            "konsole -e \"" + exe_path_ + "\" & || "
            "alacritty -e \"" + exe_path_ + "\" & || "
            "xterm -e \"" + exe_path_ + "\" &";

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

    // Poner el nombre de ventana
    setConsoleTitle(AppName_);

    return tried_to_launch_console_;
}

void ConsoleMgr::setConsoleTitle(std::string const& title) {
    #ifdef _WIN32
        SetConsoleTitleA(title.c_str());
    #else
        std::cout << "\033]0;" << title << "\007" << std::flush;
    #endif
}


// Comandos -----------------------------------------------------------------------------

void ConsoleMgr::execute_command(std::string const& command) {

    // tratar como minúsculas siempre
    std::string cmd = command;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Obtener las palabras por separado
    std::vector<std::string> tokens = tokenize_cli(cmd);

    // Comprobar que hay palabras
    if (tokens.empty())
        return;

    std::string& name = tokens[0];

    // reemplazar aliases
    if (name == "sound" || name == "snd") name = "sounds";
    if (name == "tmx")  name = "totalmix";
    if (name == "sym")  name = "symetrix";
    if (name == "h")    name = "help";
    if (name == "cls")  name = "clear";

    // Switch con strings -> Tabla de dispatch: nombre de comando -> handler(tokens).
    // Para añadir un comando nuevo basta con añadir una entrada aquí.
    using CliHandler = std::function<void(std::vector<std::string> const&)>;
    static const std::unordered_map<std::string, CliHandler> handlers = {

        { "help", [this](std::vector<std::string> const&) {
            print("Available commands:\n"
                  "  help                   - Show this help message\n"
                  "  status                 - Show app general status\n"
                  "  {clear|cls}            - Clear the terminal screen\n"
                  "  {sounds|snd} [args]    - Execute sound-related commands\n"
                  "  {symetrix|sym} [args]  - Execute symetrix-related commands\n"
                  "  {totalmix|tmx} [args]  - Execute totalmix-related commands\n"
                  "  exit                   - Exit the application\n"
                  "\n"
                  "Use '[command] help' for more information about a command"
            );
        }},

        { "clear", [this](std::vector<std::string> const& tokens) {
            if (tokens.size() == 2 && tokens[1] == "help") {
                print("Clears the terminal screen\n");
                return;
            }
            // Secuencia ANSI: \033[2J (Limpia pantalla) + \033[H (Mueve cursor a 0,0)
            print("\033[2J\033[H");
        }},

        { "status", [this](std::vector<std::string> const& tokens) {
            if (tokens.size() == 1) {
                // Lambda que valida nullptr e inicialización
                auto getModStatus = [](auto* mod) -> std::string {
                    if (!mod) return "NULL";
                    return mod->isInitialized() ? "INITIALIZED" : "FAIL";
                };

                print(
                    "--- Module status:\n"
                    "    Network module:     " + getModStatus(ctrl_->getNetModule()) + "\n"
                    "    Sounds module:      " + getModStatus(ctrl_->getSoundsModule()) + "\n"
                    "    Totalmix module:    " + getModStatus(ctrl_->getTotalmixModule()) + "\n"
                    "    Symetrix module:    " + getModStatus(ctrl_->getSymetrixModule()) + "\n"
                    "--- App status:\n"
                    "    Online mode:        " + std::string(ctrl_->isOnlineMode() ? "YES" : "NO")
                );
                return;
            }

            if (tokens.size() == 2 && tokens[1] == "help") {
                print("Shows app general status\n");
                return;
            }

            // Protección
            const auto subcommand = (tokens.size() > 1) ? tokens[1] : "<missing>";
            print("Unknown subcommand: " + subcommand);
        }},

        { "sounds", [this](std::vector<std::string> const& t) {
            execute_sounds_command(t);
        }},

        { "symetrix", [this](std::vector<std::string> const& t) {
            execute_symetrix_command(t);
        }},

        { "totalmix", [this](std::vector<std::string> const& t) {
            execute_totalmix_command(t);
        }},

        { "exit", [this](std::vector<std::string> const&) {
            SYS_INFO("AppController", "Exiting application...");
            running_ = false;
        }},

    };

    auto it = handlers.find(name);
    if (it != handlers.end())
        it->second(tokens);
    else
        print_error("Unknown command: " + name); 
}

void ConsoleMgr::execute_sounds_command(std::vector<std::string> const& tokens) {

    if (tokens.size() <= 1 || tokens[1] == "help") {
        print("Available 'sounds' commands:\n"
              "  help                            - Show this help message\n"
              "  {devices|device|dev|d} [type]   - List available devices (type: pb|cap)\n"
              "  {players|player|p} [args]       - Execute commands to player modules\n"
              "\n"
              "Aliases for 'sounds': sound, sounds, snd\n"
              "Use 'sounds [command] help' for more information about a command");
        return;
    }

    // Guard de seguridad contra puntero nulo
    SoundMgr* snd = ctrl_ ? ctrl_->getSoundsModule() : nullptr;
    if (!snd) {
        print_error("Sounds module is not available (null)");
        return;
    }

    // Subcomando: devices / device / dev
    if (tokens[1] == "devices" || tokens[1] == "device" || tokens[1] == "dev" || tokens[1]=="d") {

        // Rellenar el tercer argumento para evitar excepción
        std::string const sub = (tokens.size() > 2) ? tokens[2] : "";

        // Manejo del subcomando help
        if (sub == "help") {
            print("Usage: sounds players list [audio|morse|tts]\n"
                    "  List all registered player modules or filter by type.\n\n"
                    "Options:\n"
                    "  (none) - List audio, morse, and tts players\n"
                    "  audio  - List only audio players\n"
                    "  morse  - List only morse players\n"
                    "  tts    - List only text-to-speech players");
            return;
        }

        // Comprobar el tercer argumento
        const bool isPb  = (sub == "pb"  || sub == "playbacks" || sub == "playback");
        const bool isCap = (sub == "cap" || sub == "captures"  || sub == "capture");

        // Si hay un tercer argumento pero no es una opción válida, salta el error de comando desconocido
        if (!sub.empty() && !isPb && !isCap) {
            print_error("Unknown sounds subcommand: " + sub);
            return;
        }

        std::string output;

        if (sub.empty() || isPb) {
            output += "--- Available playback devices\n";
            for (auto const& name : snd->getAvailablePlaybacks())
                output += "  * " + name + "\n";
        }

        if (sub.empty() || isCap) {
            output += "--- Available capture devices\n";
            for (auto const& name : snd->getAvailableCaptures())
                output += "  * " + name + "\n";
        }

        print(output);
        return;
    }

    // Subcomando: players / player
    if (tokens[1] == "players" || tokens[1] == "player" || tokens[1] == "p") {

        if (tokens.size() <= 2 || tokens[2] == "help" || tokens[2] == "h") {
            print("Available 'sounds players' commands:\n"
                  "  list                - List all available player modules\n"
                  "  {morse|m} [args]    - Execute commands to morse players\n"
                  "  help                - Show this help message");
            return;
        }

        if (tokens[2] == "list") {
            std::string const sub = (tokens.size() > 3) ? tokens[3] : "";

            const bool isAudio = (sub == "audio");
            const bool isMorse = (sub == "morse");
            const bool isTTS   = (sub == "tts");

            // Si hay un cuarto token y no coincide con ninguna opción válida
            if (!sub.empty() && !isAudio && !isMorse && !isTTS) {
                print_error("Unknown player type: " + sub);
                return;
            }

            std::string output;

            if (sub.empty() || isAudio) {
                output += "--- Available audio player modules ---\n";
                for (auto const& name : snd->getPlayerAudioNames())
                    output += "  * " + name + "\n";
            }

            if (sub.empty() || isMorse) {
                output += "--- Available morse player modules ---\n";
                for (auto const& name : snd->getPlayerMorseNames())
                    output += "  * " + name + "\n";
            }

            if (sub.empty() || isTTS) {
                output += "--- Available tts player modules ---\n";
                for (auto const& name : snd->getPlayerTTSNames())
                    output += "  * " + name + "\n";
            }

            print(output);
            return;
        }

        if (tokens[2] == "morse" || tokens[2] == "m") {

            if (tokens.size() <= 3 || tokens[3] == "help" || tokens[3] == "h") {
                print("Available 'sounds players morse' commands:\n\n"
                    "  {help|h}\n"
                    "      - Show this help message\n\n"
                    "  {add|a} <name> [params...]\n"
                    "      - Add a new morse player\n\n"
                    "  {info|i} <name>\n"
                    "      - Show detailed information about a morse player\n\n"
                    "  {use|u} <name> play <text>\n"
                    "      - Play morse code using the specified player\n\n"
                    "  {remove|delete|r|d} <name>\n"
                    "      - Remove the specified morse player"
                );
                return;
            }

            // Comprobar tamaño
            if (tokens.size() <= 4) {
                print_error("Enter the name of the player morse");
                return;
            }
            // Rellenar el tercer argumento para evitar excepción
            std::string const& playerName = (tokens.size() > 4) ? tokens[4] : "";
            
            if (tokens[3] == "add" || tokens[3] =="a") {
                if(snd->addPlayerMorse(nullptr, playerName))
                    print("Morse Player " + playerName + " added");
                else print_error("Cannot add morse player");
                return;
            }

            if (tokens[3] == "remove" || tokens[3] == "remove" || tokens[3] == "r" || tokens[3] == "d") {
                if(snd->removePlayerMorse(playerName))
                    print("Morse Player " + playerName + " removed");
                else print_error("Cannot remove morse player");

                return;
            }

            if (tokens[3] == "use" || tokens[3] == "u") {

                PlayerMorse* pm = snd->getPlayerMorse(playerName);
                if (!pm) {
                    print_error("Morse module '" + playerName + "' not available.");
                    return;
                }

                if (tokens[5] == "play") {

                    if (tokens.size() <= 6) {
                        print_error("Enter the morse text to play");
                        return;
                    }

                    // Reproducir morse
                    std::string const& text = tokens[6];
                    if(pm)
                        pm->playMorse(text);

                    return;
                }
                if (tokens[5] == "info" || tokens[5] == "i") {
                    print("Show info not implemented");
                    return;
                }
                else {
                    print_error("Unknown sounds subcommand: " + tokens[5]);
                    return;
                }

                return;
            }

            else {
                print_error("Unknown sounds subcommand: " + tokens[3]);
                return;
            }
        }

        else {
            print_error("Unknown sounds subcommand: " + tokens[2]);
            return;
        }
    }

    print_error("Unknown sounds subcommand: " + tokens[1]);
}

void ConsoleMgr::execute_totalmix_command(std::vector<std::string> const& tokens) {

    if (tokens.size() <= 1 || tokens[1] == "help") {
        print("Available 'totalmix' commands:\n"
              "  totalmix status                 - Show module status\n"
              "  totalmix volume <out> <value>   - Set output volume (0-100)\n"
              "  totalmix threshold <in> <value> - Set threshold value (0-100)\n"
              "  totalmix mute <out>             - Mute an output\n"
              "  totalmix unmute <out>           - Unmute an output");
        return;
    }

    // Obtener el módulo de Totalmix y validar nullptr
    TotalMix* tmx = ctrl_ ? ctrl_->getTotalmixModule() : nullptr;

    if (tokens[1] == "status") {
        const std::string str = (tmx && tmx->isInitialized()) ? "INITIALIZED" : "FAIL";
        print("--- Totalmix module status: " + str);
        return;
    }

    // Si el módulo no está disponible o inicializado para el resto de subcomandos
    if (!tmx || !tmx->isInitialized()) {
        print_error("Totalmix module not initialized or unavailable");
        return;
    }

    if (tokens[1] == "volume") {
        if (tokens.size() < 4) {
            print_error("Usage: totalmix volume <out> <value>");
            return;
        }

        try {
            int   out   = std::stoi(tokens[2]);
            float value = std::stof(tokens[3]);
            if (!tmx->SetOutputVolume(out, value))
                SYS_WARN("CLI", "Failed to set Totalmix output volume.");
        } catch (std::exception const&) {
            print_error("Invalid <out>/<value>, must be numeric.");
        }
        return;
    }

    if (tokens[1] == "threshold") {
        if (tokens.size() < 4) {
            print_error("Usage: totalmix threshold <in> <value>");
            return;
        }

        try {
            int   in   = std::stoi(tokens[2]);
            float value = std::stof(tokens[3]);
            if (!tmx->SetInputThreshold(in, value))
                SYS_WARN("CLI", "Failed to set Totalmix input threshold");
        } catch (std::exception const&) {
            print_error("Invalid <in>/<value>, must be numeric.");
            SYS_WARN("CLI", "Invalid <in>/<value>, must be numeric.");
        }
        return;
    }

    if (tokens[1] == "mute" || tokens[1] == "unmute") {
        if (tokens.size() < 3) {
            print_error("Usage: totalmix " + tokens[1] + " <out>");
            return;
        }

        try {
            int out = std::stoi(tokens[2]);
            if (!tmx->SetMuteOutput(out, tokens[1] == "mute"))
                SYS_WARN("CLI", "Failed to set Totalmix mute state.");
        } catch (std::exception const&) {
            print_error("Invalid <out>, must be numeric.");
        }
        return;
    }

    print_error("Unknown totalmix subcommand: " + tokens[1]);
}

void ConsoleMgr::execute_symetrix_command(std::vector<std::string> const& tokens) {

    std::string const second_token = (tokens.size() > 1) ? tokens[1] : "help";

    if (second_token == "help") {
        print("Available 'symetrix' commands:\n"
              "  status  - Show connection status");
        return;
    }

    // Obtener el módulo de symetrix con guard de nulidad
    Symetrix* sym = ctrl_ ? ctrl_->getSymetrixModule() : nullptr;

    if (second_token == "status") {
        const std::string connStr = (sym && sym->isConnected()) ? "YES" : "NO";
        print("Symetrix connected: " + connStr);
        return;
    }

    print_error("Unknown symetrix subcommand: " + second_token);
}


// Tokens (comandos) --------------------------------------------------------------------

std::vector<std::string> ConsoleMgr::tokenize_cli(std::string const& line) {
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

std::unordered_map<std::string, std::string> ConsoleMgr::parse_flags(
    std::vector<std::string> const& tokens, size_t size)
{
    // Mapa a devolver
    std::unordered_map<std::string, std::string> flags;

    // Recorrer el vector de palabras
    for (size_t token = size; token < tokens.size(); ++token)

        // Si tiene --, es una clave para el mapa
        if (tokens[token].rfind("--", 0) == 0) {

            // Coge el valor siguiente como valor del mapa
            std::string value = (token + 1 < tokens.size()) ? tokens[token + 1] : "";
            flags[tokens[token]] = value;

            // saltar el valor ya consumido
            ++token;
        }
    return flags;
}


// Imprimir texto -----------------------------------------------------------------------

void ConsoleMgr::print(std::string const& txt) {
    std::cout << ANSI_BOLD << txt << ANSI_RESET << "\n" << std::endl;
}

void ConsoleMgr::print_error(std::string const& txt) {
    std::cerr << ANSI_BOLD << ANSI_RED << txt << ANSI_RESET << "\n" << std::endl;
}
