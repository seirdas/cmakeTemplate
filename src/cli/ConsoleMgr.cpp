#include "cli/ConsoleMgr.hpp"
#include "app/IModule.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "system/ANSI.hpp"

// Módulos
#include "app/IAppControl.hpp"
#include "sound/SoundMgr.hpp"
#include "sound/PlayerAudio.hpp"
#include "sound/PlayerMorse.hpp"
#include "sound/PlayerTTS.hpp"
#include "sound/AudioPlaybackModule.hpp"
#include "devices/TotalMix.hpp"
#include "devices/Symetrix.hpp"
#include "net/NetMgr.hpp"
#include "net/UdpSocket.hpp"

// Otros
#include <iostream>
#include <functional>


// General ------------------------------------------------------------------------------

ConsoleMgr::ConsoleMgr() :
    IModule(),
    AppName_("app"),
    exe_path_(""),
    cli_running_(false),
    tried_to_launch_console_(false),
    show_app_name_(true),
    prefix_color_(ANSI_BRIGHT_BLUE)
{

}

ConsoleMgr::~ConsoleMgr() {
    close();
}


// Métodos comunes de módulo (IModule) --------------------------------------------------

bool ConsoleMgr::init(void* config) {
    if (!IModule::init(config)) {
        SYS_WARN("NetMgr", "Error in base initialization");
        return false;
    }
    
    return true;
}

void ConsoleMgr::loadConfig(void* config) {

    // Comprobar si existe previamente
    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // Obtener configuración
    jsonMgr.get_or_set(cfg, "app_name_window",	AppName_);
    jsonMgr.get_or_set(cfg, "show_app_name",	show_app_name_);

    // Color
    std::string color = "light blue";
    jsonMgr.get_or_set(cfg, "prefix_color",	color);
    
    // Colores estándar
    if (color == "black") prefix_color_              = ANSI_BLACK;
    else if (color == "red") prefix_color_           = ANSI_RED;
    else if (color == "green") prefix_color_         = ANSI_GREEN;
    else if (color == "yellow") prefix_color_        = ANSI_YELLOW;
    else if (color == "blue") prefix_color_          = ANSI_BLUE;
    else if (color == "magenta") prefix_color_       = ANSI_MAGENTA;
    else if (color == "cyan") prefix_color_          = ANSI_CYAN;
    else if (color == "white") prefix_color_         = ANSI_WHITE;
    
    // Colores brillantes (Bright/Light)
    else if (color == "bright red" || color == "light red")       prefix_color_ = ANSI_BRIGHT_RED;
    else if (color == "bright green" || color == "light green")   prefix_color_ = ANSI_BRIGHT_GREEN;
    else if (color == "bright yellow" || color == "light yellow") prefix_color_ = ANSI_BRIGHT_YELLOW;
    else if (color == "bright blue" || color == "light blue")     prefix_color_ = ANSI_BRIGHT_BLUE;
    else if (color == "bright magenta" || color == "light magenta") prefix_color_ = ANSI_BRIGHT_MAGENTA;
    else if (color == "bright cyan" || color == "light cyan")     prefix_color_ = ANSI_BRIGHT_CYAN;

    // Color por defecto
    else prefix_color_ = ANSI_BRIGHT_BLUE;

}

bool ConsoleMgr::close() {
    // Ejecutar cierre común (cambia flags, etc.)
    if (!IModule::close())
        return false;
    
    return true;
}


// Ejecución ----------------------------------------------------------------------------

bool ConsoleMgr::Run() {

    // Inicializar consola
    LaunchConsole();

    // Cambiar el título de la consola
    setConsoleTitle(AppName_);
    
    // Info
    SYS_INFO("ConsoleMgr", "Running in CLI Mode. Type 'help' for commands or 'exit' to quit.");
    cli_running_ = true;

    std::string command;
    size_t      cursor = 0; // Posición del cursor dentro de "command" (no siempre al final)

    // Historial de comandos (flechas arriba/abajo)
    std::vector<std::string> history;
    size_t                   historyIndex = 0; // == history.size() cuando no se está navegando
    std::string              draft;            // Línea en curso, guardada al empezar a navegar el historial

    // Ocultar cursor en terminales ANSI para eliminar flicker visual
    //std::cerr << "\033[?25l";

    SystemMgr::instance().setCliActive(true);
    SystemMgr::instance().showAppName(show_app_name_);
    SystemMgr::instance().updateCliInput(command, cursor, prefix_color_); // Pinta el primer prompt

    #ifndef _WIN32
        // LINUX: Apagar el "Cooked Mode" y el "Echo" automático de la terminal
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    #endif

    while (cli_running_) {
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

            execute_cmd(command);

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

void ConsoleMgr::setAppName(std::string const& appName) {
    AppName_ = appName;
}

bool ConsoleMgr::LaunchConsole(bool force) {

    // Si ya lo ha intentado, no hacerlo de nuevo (evita errores duplicados)
    if (tried_to_launch_console_ && !force)
        return true;
    tried_to_launch_console_ = true;

    SYS_INFO("ConsoleMgr", "Trying to launch console...");

    bool success = false;

    #ifdef _WIN32

        // Comprobar si ya hay una terminal
        if (GetConsoleWindow() != NULL) {
            SYS_INFO("ConsoleMgr", "Console already opened");
            return false;
        }

        // Abrir terminal en Windows
        if (!AllocConsole()) {
            SYS_WARN("ConsoleMgr", "Failed to alloc console");
            return false;
        }

        // Probar escritura
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONIN$", "r", stdin);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        
        // Forzar la reasignación de los búferes de std::cout, std::cerr y std::cin
        static std::ofstream outConsole("CONOUT$");
        static std::ofstream errConsole("CONOUT$");
        static std::ifstream inConsole("CONIN$");

        std::cout.rdbuf(outConsole.rdbuf());
        std::cerr.rdbuf(errConsole.rdbuf());
        std::cin.rdbuf(inConsole.rdbuf());
        
        // Desincronizar C y C++ stream buffers e inicializar estados
        std::ios::sync_with_stdio();
        std::cin.clear();
        std::cout.clear();
        std::cerr.clear();

        // Activar procesamiento de colores ANSI
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
        
        std::cout << std::endl;
        SYS_INFO("ConsoleMgr", "Console allocated successfully");
        success = true;

    #else

        // Si la aplicación no tiene una TTY estándar asignada
        if (isatty(STDIN_FILENO)) {
            SYS_INFO("ConsoleMgr", "Console already opened.");
            return false;
        }

        SYS_INFO("ConsoleMgr", "No TTY detected. Launching external terminal window...");

        // Lanzar terminal ejecutando binario dentro de ella
        std::string cmd = 
            "qterminal -e \"" + exe_path_ + "\" & || "
            "xfce4-terminal -e \"" + exe_path_ + "\" & || "
            "konsole -e \"" + exe_path_ + "\" & || "
            "alacritty -e \"" + exe_path_ + "\" & || "
            "xterm -e \"" + exe_path_ + "\" &";

        int ret = std::system(cmd.c_str());

        if (ret == 0) {
            SYS_INFO("ConsoleMgr", "Delegated execution to new terminal window. Exiting background process.");
            std::exit(0); 
        }

        SYS_ERROR("ConsoleMgr", "Failed to launch external terminal.");
        return false;

    #endif

    if (success) {
        // Poner el nombre de ventana (ahora sí se ejecuta en Windows)
        setConsoleTitle(AppName_);
    }

    return success;
}

void ConsoleMgr::setConsoleTitle(std::string const& title) {
    #ifdef _WIN32
        SetConsoleTitleA(title.c_str());
    #else
        std::cout << "\033]0;" << title << "\007" << std::flush;
    #endif
}


// Comandos -----------------------------------------------------------------------------

void ConsoleMgr::execute_cmd(std::string const& command) {

    // tratar como minúsculas siempre
    std::string cmd = command;
    for (char &c : cmd)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); 

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
    if (name == "net")  name = "network";
    if (name == "h")    name = "help";
    if (name == "cls")  name = "clear";

    // Switch con strings -> Tabla de dispatch: nombre de comando -> handler(tokens).
    // Para añadir un comando nuevo basta con añadir una entrada aquí.
    using CliHandler = std::function<void(std::vector<std::string> const&)>;
    static const std::unordered_map<std::string, CliHandler> handlers = {

        { "help", [this](std::vector<std::string> const&) {
            print("Available commands:\n"
                  "  help, h                -- Show this help message\n"
                  "  status                 -- Show app general status\n"
                  "  version                -- Show app version\n"
                  "  clear, cls             -- Clear the terminal screen\n"
                  "  sounds, snd <args>     -- Execute sound-related commands\n"
                  "  symetrix, sym <args>   -- Execute symetrix-related commands\n"
                  "  totalmix, tmx <args>   -- Execute totalmix-related commands\n"
                  "  network, net <args>    -- Execute network-related commands (add, devices, remove)\n"
                  "  online <args>          -- Switch online/offline mode\n"
                  "  exit                   -- Exit the application\n"
                  "\n"
                  "Use '[command] help' for more information about a command"
            );
        }},

        { "clear", [this](std::vector<std::string> const& tokens) {
            if (tokens.size() == 2 && tokens[1] == "help") {
                print("Clears the terminal screen.\n");
                return;
            }
            // Secuencia ANSI: \033[2J (Limpia pantalla) + \033[H (Mueve cursor a 0,0)
            print("\033[2J\033[H");
        }},

        { "version", [this](std::vector<std::string> const& tokens) {
            if (tokens.size() == 2 && tokens[1] == "help") {
                print("Show app version.\n");
                return;
            }
            // Secuencia ANSI: \033[2J (Limpia pantalla) + \033[H (Mueve cursor a 0,0)
            print(AppName_ + " version: " + ctrl_->getVersion());
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
            execute_cmd_snd(t);
        }},

        { "symetrix", [this](std::vector<std::string> const& t) {
            execute_symetrix_command(t);
        }},

        { "totalmix", [this](std::vector<std::string> const& t) {
            execute_totalmix_command(t);
        }},

        { "network", [this](std::vector<std::string> const& t) {
            execute_cmd_net(t);
        }},

        { "online", [this](std::vector<std::string> const& t) {
            execute_cmd_online(t);
        }},

        { "exit", [this](std::vector<std::string> const&) {
            SYS_INFO("ConsoleMgr", "Exiting application...");
            cli_running_ = false;
        }},

    };

    auto it = handlers.find(name);
    if (it != handlers.end())
        it->second(tokens);
    else
        print_error("Unknown command: " + name);
}

void ConsoleMgr::execute_cmd_online(std::vector<std::string> const& tokens) {
    const std::string subCmd = get_token(tokens, 1);

    if (subCmd.empty() || subCmd == "help") {
        print("Available 'online' commands:\n"
              "  online on       - Switch to online mode\n"
              "  online off      - Switch to offline mode\n"
              "  online toggle   - Switch to the opposite mode\n\n"
              "Use 'status' to check the current online/offline mode.");
        return;
    }

    if (!ctrl_) {
        print_error("App controller is not available (null)");
        return;
    }

    if (subCmd == "on") {
        ctrl_->setOnlineMode(true);
        print("Switched to ONLINE mode.");
    } else if (subCmd == "off") {
        ctrl_->setOnlineMode(false);
        print("Switched to OFFLINE mode.");
    } else if (subCmd == "toggle") {
        const bool newMode = !ctrl_->isOnlineMode();
        ctrl_->setOnlineMode(newMode);
        print(std::string("Switched to ") + (newMode ? "ONLINE" : "OFFLINE") + " mode.");
    } else {
        print_error("Unknown online subcommand: " + subCmd);
    }
}


// Subcomandos Network ------------------------------------------------------------------

void ConsoleMgr::execute_cmd_snd(std::vector<std::string> const& tokens) {
    const std::string subCmd = get_token(tokens, 1);

    if (subCmd.empty() || subCmd == "help" || subCmd == "h") {
        print("Available 'sounds' commands:\n"
            "  help                               - Show this help message\n"
            "  devices, device, dev, d [type]     - List available devices (type: pb|cap)\n"
            "  {players|player|p} [args]          - Execute commands to player modules\n"
            "      -> then {audio|a} / {morse|m} / {tts|t} to manage each player type\n"
            "      -> e.g. 'sounds players morse help' for morse-specific commands\n\n"
            "Aliases: sound, sounds, snd"
            "\n"
            "Use 'sounds [command] help' for more information about a command"
        );
        return;
    }

    SoundMgr* snd = ctrl_ ? ctrl_->getSoundsModule() : nullptr;
    if (!snd) {
        print_error("Sounds module is not available (null)");
        return;
    }

    if (subCmd == "devices" || subCmd == "device" || subCmd == "dev" || subCmd == "d")
        execute_cmd_snd_devices(tokens);
    else if (subCmd == "players" || subCmd == "player" || subCmd == "p")
        execute_cmd_snd_players(tokens);
    else
        print_error("Unknown sounds subcommand: " + subCmd);
}

void ConsoleMgr::execute_cmd_snd_devices(std::vector<std::string> const& tokens) {
    const std::string subCmd = get_token(tokens, 2);

    SoundMgr* snd = ctrl_ ? ctrl_->getSoundsModule() : nullptr;
    if (!snd) {
        print_error("Sounds module is not available (null)");
        return;
    }

    if (subCmd == "help") {
        print("Usage: sounds devices [pb|cap]\n"
              "  pb  - List playback devices\n"
              "  cap - List capture devices\n");
        return;
    }

    const bool isPb  = (subCmd == "pb"  || subCmd == "playbacks" || subCmd == "playback");
    const bool isCap = (subCmd == "cap" || subCmd == "captures"  || subCmd == "capture");

    if (!subCmd.empty() && !isPb && !isCap) {
        print_error("Unknown devices option: " + subCmd);
        return;
    }

    std::string output;
    if (subCmd.empty() || isPb) {
        output += "--- Available playback devices ---\n";
        for (auto const& name : snd->getAvailablePlaybacks()) output += "  * " + name + "\n";
        if (subCmd.empty()) output += "\n";
    }
    if (subCmd.empty() || isCap) {
        output += "--- Available capture devices ---\n";
        for (auto const& name : snd->getAvailableCaptures()) output += "  * " + name + "\n";
    }

    print(output);
}

void ConsoleMgr::execute_cmd_snd_players(std::vector<std::string> const& tokens) {
    const std::string category = get_token(tokens, 2);

    SoundMgr* snd = ctrl_->getSoundsModule();
    if(!snd) {
        print_error("Sounds module not found (nullptr)");
        return;
    }

    if (category.empty() || category == "help" || category == "h") {
        print("Available 'sounds players' commands:\n"
              "  list                - List all player modules\n"
              "  {audio|a} [args]    - Commands for audio players\n"
              "  {morse|m} [args]    - Commands for morse players\n"
              "  {tts|t} [args]      - Commands for tts players");
        return;
    }

    if (category == "list") {
        const std::string filter = get_token(tokens, 3);
        const bool isAudio = (filter == "audio");
        const bool isMorse = (filter == "morse");
        const bool isTTS   = (filter == "tts");

        if (!filter.empty() && !isAudio && !isMorse && !isTTS) {
            print_error("Unknown player filter: " + filter);
            return;
        }

        std::string output;
        auto appendList = [&](const char* title, const auto& list) {
            output += std::string("--- ") + title + " ---\n";
            for (auto const& name : list) output += "  * " + name + "\n";
            output += "\n";
        };

        if (filter.empty() || isAudio) appendList("Audio modules", snd->getPlayerAudioNames());
        if (filter.empty() || isMorse) appendList("Morse modules", snd->getPlayerMorseNames());
        if (filter.empty() || isTTS)   appendList("TTS modules",   snd->getPlayerTTSNames());

        print(output);
        return;
    }

    if (category == "morse" || category == "m") execute_cmd_snd_morse(tokens);
    else if (category == "audio" || category == "a") execute_cmd_snd_audio(tokens);
    else if (category == "tts" || category == "t") execute_cmd_snd_tts(tokens);
    else print_error("Unknown players category: " + category);
}

void ConsoleMgr::execute_cmd_snd_morse(std::vector<std::string> const& tokens) {
    const std::string action = get_token(tokens, 3);

    SoundMgr* snd = ctrl_->getSoundsModule();

    if (action.empty() || action == "help" || action == "h") {
        print("Available 'sounds players morse' commands:\n\n"
              "  {add|a} <name>\n"
              "      - Add a new morse player\n"
              "      Ex: sounds players morse add speaker\n\n"
              "  {remove|delete|r|d} <name>\n"
              "      - Remove the specified morse player\n"
              "      Ex: sounds players morse remove speaker\n\n"
              "  {use|u} <name> play <text>\n"
              "      - Play morse code using the specified player (audioName == text)\n"
              "      Ex: sounds players morse use speaker play sos\n\n"
              "  {use|u} <name> stop <text> [force] [fadeOutMs] [pitchOutMs]\n"
              "      - Stop the given morse sound. Soft by default: if it's not looping\n"
              "        and 'force' isn't set, it just lets it finish naturally.\n"
              "        force=true/1 cuts it immediately; fadeOutMs/pitchOutMs fade it out\n"
              "      Ex: sounds players morse use speaker stop sos true 500\n\n"
              "  {use|u} <name> volume <text> <0-100>\n"
              "      - Set the volume of that specific playing sound\n"
              "      Ex: sounds players morse use speaker volume sos 50\n\n"
              "  {use|u} <name> modulevolume <0-100>\n"
              "      - Set the player's global volume (affects all sounds playing on it)\n"
              "      Ex: sounds players morse use speaker modulevolume 80\n\n"
              "  {use|u} <name> pitch <text> <value>\n"
              "      - Set the pitch of that specific playing sound\n"
              "      Ex: sounds players morse use speaker pitch sos 1.2\n\n"
              "  {use|u} <name> channel <0|1|2>\n"
              "      - Select output channel: 0 = both, 1 = left, 2 = right\n"
              "      Ex: sounds players morse use speaker channel 1\n\n"
              "  {use|u} <name> isplaying [text]\n"
              "      - Check if a specific sound (or anything) is playing on this player\n"
              "      Ex: sounds players morse use speaker isplaying sos"
        );
        return;
    }

    const std::string name = get_token(tokens, 4);
    if (name.empty()) {
        print_error("Player name is required.");
        return;
    }

    if (action == "add" || action == "a") {
        if (snd->addPlayerMorse(nullptr, name)) print("Morse player '" + name + "' added.");
        else print_error("Cannot add morse player '" + name + "'");
    } 
    else if (action == "remove" || action == "delete" || action == "r" || action == "d") {
        if (snd->removePlayerMorse(name)) print("Morse player '" + name + "' removed.");
        else print_error("Cannot remove morse player '" + name + "'");
    }
    else if (action == "use" || action == "u") {
        PlayerMorse* pm = snd->getPlayerMorse(name);
        if (!pm) { print_error("Morse player '" + name + "' not found."); return; }

        const std::string subAction = get_token(tokens, 5);
        if (subAction == "play") {
            const std::string text = get_token(tokens, 6);
            if (text.empty()) { print_error("Enter the text to play."); return; }
            pm->playMorse(text);
        } else if (subAction == "info" || subAction == "i") {
            print("Info not implemented.");
        } else if (!execute_playback_command(pm, tokens, 5)) {
            print_error("Unknown use subcommand: " + subAction);
        }
    }
    else {
        print_error("Unknown action: " + action);
    }
}

void ConsoleMgr::execute_cmd_snd_audio(std::vector<std::string> const& tokens) {
    const std::string action = get_token(tokens, 3);
    if (action.empty() || action == "help" || action == "h") {
        print("Available 'sounds players audio' commands:\n\n"
              "  {add|a} <name> [device]\n"
              "      - Add a new audio player, optionally bound to a playback device\n"
              "      Ex: sounds players audio add speaker\n\n"
              "  {remove|delete|r|d} <name>\n"
              "      - Remove the specified audio player\n"
              "      Ex: sounds players audio remove speaker\n\n"
              "  {use|u} <name> play <filepath> [volume]\n"
              "      - Play an audio file (audioName == filepath)\n"
              "      Ex: sounds players audio use speaker play song.wav 80\n\n"
              "  {use|u} <name> stop <filepath> [force] [fadeOutMs] [pitchOutMs]\n"
              "      - Stop the given sound. Soft by default: if it's not looping\n"
              "        and 'force' isn't set, it just lets it finish naturally.\n"
              "        force=true/1 cuts it immediately; fadeOutMs/pitchOutMs fade it out\n"
              "      Ex: sounds players audio use speaker stop song.wav true 500\n\n"
              "  {use|u} <name> volume <filepath> <0-100>\n"
              "      - Set the volume of that specific playing sound\n"
              "      Ex: sounds players audio use speaker volume song.wav 50\n\n"
              "  {use|u} <name> modulevolume <0-100>\n"
              "      - Set the player's global volume (affects all sounds playing on it)\n"
              "      Ex: sounds players audio use speaker modulevolume 80\n\n"
              "  {use|u} <name> pitch <filepath> <value>\n"
              "      - Set the pitch of that specific playing sound\n"
              "      Ex: sounds players audio use speaker pitch song.wav 1.2\n\n"
              "  {use|u} <name> channel <0|1|2>\n"
              "      - Select output channel: 0 = both, 1 = left, 2 = right\n"
              "      Ex: sounds players audio use speaker channel 1\n\n"
              "  {use|u} <name> isplaying [filepath]\n"
              "      - Check if a specific sound (or anything) is playing on this player\n"
              "      Ex: sounds players audio use speaker isplaying song.wav\n\n"
              "Note: a player can have several sounds playing at once (it's a mixer),\n"
              "  so stop/volume/pitch/isplaying need the exact filepath used in 'play'."
        );
        return;
    }

    SoundMgr* snd = ctrl_->getSoundsModule();

    const std::string name = get_token(tokens, 4);
    if (name.empty()) { print_error("Player name required."); return; }

    if (action == "add" || action == "a") {
        std::string device = get_token(tokens, 5);
        // #TODO Revisar
        //if (snd->addPlayerAudio(nullptr, name)) print("Audio player '" + name + "' added.");
        //else print_error("Cannot add audio player.");
    } 
    else if (action == "remove" || action == "delete" || action == "r" || action == "d") {
        if (snd->removePlayerAudio(name)) print("Audio player '" + name + "' removed.");
        else print_error("Cannot remove audio player.");
    } 
    else if (action == "use" || action == "u") {
        PlayerAudio* pa = snd->getPlayerAudio(name);
        if (!pa) { print_error("Audio player '" + name + "' not found."); return; }

        if (get_token(tokens, 5) == "play") {
            std::string filepath = get_token(tokens, 6);
            if (filepath.empty()) { print_error("Filepath required."); return; }
            
            unsigned short vol = 100;
            std::string volStr = get_token(tokens, 7);
            if (!volStr.empty()) {
                try { vol = static_cast<unsigned short>(std::stoi(volStr)); }
                catch (...) { print_error("Invalid volume."); return; }
            }
            // #TODO revisar
            // pa->playAudio(filepath, vol);
        } else if (!execute_playback_command(pa, tokens, 5)) {
            print_error("Unknown use subcommand: " + get_token(tokens, 5));
        }
    }
}

void ConsoleMgr::execute_cmd_snd_tts(std::vector<std::string> const& tokens) {
    const std::string action = get_token(tokens, 3);
    if (action.empty() || action == "help" || action == "h") {
        print("Available 'sounds players tts' commands:\n\n"
              "  {add|a} <name> [device]\n"
              "      - Add a new tts player, optionally bound to a playback device\n\n"
              "  {remove|delete|r|d} <name>\n"
              "      - Remove the specified tts player\n\n"
              "  {use|u} <name> play <model> <audioName> <text>\n"
              "      - Play a text-to-speech phrase (audioName is a name YOU choose,\n"
              "        used afterwards with stop/volume/isplaying)\n\n"
              "  {use|u} <name> stop <audioName> [force] [fadeOutMs] [pitchOutMs]\n"
              "      - Stop the given phrase. Soft by default: if 'force' isn't set,\n"
              "        it just lets it finish naturally.\n"
              "        force=true/1 cuts it immediately; fadeOutMs/pitchOutMs fade it out\n\n"
              "  {use|u} <name> volume <audioName> <0-100>\n"
              "      - Set the volume of that specific playing phrase\n\n"
              "  {use|u} <name> modulevolume <0-100>\n"
              "      - Set the player's global volume (affects all sounds playing on it)\n\n"
              "  {use|u} <name> pitch <audioName> <value>\n"
              "      - Set the pitch of that specific playing phrase\n\n"
              "  {use|u} <name> channel <0|1|2>\n"
              "      - Select output channel: 0 = both, 1 = left, 2 = right\n\n"
              "  {use|u} <name> isplaying [audioName]\n"
              "      - Check if a specific phrase (or anything) is playing on this player"
        );
        return;
    }

    SoundMgr* snd = ctrl_->getSoundsModule();

    const std::string name = get_token(tokens, 4);
    if (name.empty()) { print_error("Player name required."); return; }

    if (action == "add" || action == "a") {
        std::string device = get_token(tokens, 5);
        // #TODO Revisar
        //if (snd->addPlayerTTS(nullptr, name)) print("TTS player '" + name + "' added.");
        //else print_error("Cannot add TTS player.");
    } 
    else if (action == "remove" || action == "delete" || action == "r" || action == "d") {
        if (snd->removePlayerTTS(name)) print("TTS player '" + name + "' removed.");
        else print_error("Cannot remove TTS player.");
    } 
    else if (action == "use" || action == "u") {

        // Obtener y comprobar el player
        PlayerTTS* pt = snd->getPlayerTTS(name);
        if (!pt) { 
            print_error("TTS player '" + name + "' not found."); 
            return; 
        }

        if (get_token(tokens, 5) == "play") {
            std::string model = get_token(tokens, 6);
            std::string audioName = get_token(tokens, 7);
            std::string text = get_token(tokens, 8);

            if (model.empty() || audioName.empty() || text.empty()) {
                print_error("Usage: sounds players tts use <name> play <model> <audioName> <text>");
                return;
            }
            
            if (!pt->playTTS(text, model, audioName)) 
                print_error("Cannot play TTS.");

        } else if (!execute_playback_command(pt, tokens, 5)) {
            print_error("Unknown use subcommand: " + get_token(tokens, 5));
        }
    }
}

bool ConsoleMgr::execute_playback_command(
    AudioPlaybackModule*             mod,
    std::vector<std::string> const&  tokens,
    size_t                           subIdx)
{
    const std::string sub = get_token(tokens, subIdx);

    if (sub == "stop") {
        const std::string audioName = get_token(tokens, subIdx + 1);
        if (audioName.empty()) {
            print_error("Usage: ... stop <audioName> [force] [fadeOutMs] [pitchOutMs]");
            return true;
        }
        const std::string forceStr = get_token(tokens, subIdx + 2);
        const bool force = (forceStr == "true" || forceStr == "1");

        unsigned int fadeOutMs  = 0;
        unsigned int pitchOutMs = 0;
        try {
            const std::string fadeStr = get_token(tokens, subIdx + 3);
            if (!fadeStr.empty()) fadeOutMs = static_cast<unsigned int>(std::stoul(fadeStr));
            const std::string pitchStr = get_token(tokens, subIdx + 4);
            if (!pitchStr.empty()) pitchOutMs = static_cast<unsigned int>(std::stoul(pitchStr));
        } catch (std::exception const&) {
            print_error("Invalid <fadeOutMs>/<pitchOutMs>, must be numeric.");
            return true;
        }
        // #TODO Revisar
        //mod->stop(audioName, force, fadeOutMs, pitchOutMs);
        return true;
    }

    if (sub == "volume") {
        const std::string audioName = get_token(tokens, subIdx + 1);
        const std::string volStr    = get_token(tokens, subIdx + 2);
        if (audioName.empty() || volStr.empty()) {
            print_error("Usage: ... volume <audioName> <0-100>");
            return true;
        }
        try {
            mod->setVolume(audioName, static_cast<unsigned short>(std::stoi(volStr)));
        } catch (std::exception const&) {
            print_error("Invalid <volume>, must be numeric.");
        }
        return true;
    }

    if (sub == "modulevolume") {
        const std::string volStr = get_token(tokens, subIdx + 1);
        if (volStr.empty()) {
            print_error("Usage: ... modulevolume <0-100>");
            return true;
        }
        try {
            mod->setModuleVolume(static_cast<unsigned short>(std::stoi(volStr)));
        } catch (std::exception const&) {
            print_error("Invalid <volume>, must be numeric.");
        }
        return true;
    }

    if (sub == "pitch") {
        const std::string audioName = get_token(tokens, subIdx + 1);
        const std::string pitchStr  = get_token(tokens, subIdx + 2);
        if (audioName.empty() || pitchStr.empty()) {
            print_error("Usage: ... pitch <audioName> <value>");
            return true;
        }
        try {
            // #TODO Revisar
            // mod->setPitch(audioName, std::stof(pitchStr));
        } catch (std::exception const&) {
            print_error("Invalid <pitch>, must be numeric.");
        }
        return true;
    }

    if (sub == "channel") {
        const std::string chStr = get_token(tokens, subIdx + 1);
        if (chStr.empty()) {
            print_error("Usage: ... channel <0|1|2>");
            return true;
        }
        try {
            // #TODO Revisar
            //mod->setSelectedChannel(static_cast<unsigned short>(std::stoi(chStr)));
            int borrame = 2;
        } catch (std::exception const&) {
            print_error("Invalid <channel>, must be numeric.");
        }
        return true;
    }

    return false;
}


// Subcomandos Network ------------------------------------------------------------------

void ConsoleMgr::execute_cmd_net(std::vector<std::string> const& tokens){
    const std::string subCmd = get_token(tokens, 1);

    if (subCmd.empty() || subCmd == "help" || subCmd == "h") {
        print("Available 'network' commands:\n"
              "  help                              - Show this help message\n"
              "  {devices|device|dev|d}            - List active UDP sockets\n"
              "  add <name> <port> [ip] [pkgsize]  - Add a new UDP socket\n"
              "  {remove|delete|rm} <name>         - Remove an existing UDP socket\n"
              "  {status|st}                       - Show whether the network is running\n"
              "  {exists|e} <name>                 - Check if a socket exists\n"
              "  {lastpacket|lp} <name>            - Time since the last packet on a socket\n"
              "  {send|s} <name> <ip> <port> <text> - Send text data through a socket\n"
              "  {queue|q}                         - Number of packets pending in the receive queue\n\n"
              "Note: the network module starts/stops automatically with online/offline mode\n"
              "  (see 'status'); it's not controlled manually from here.\n\n"
              "Aliases: net, network");
        return;
    }

    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    if (subCmd == "devices" || subCmd == "device" || subCmd == "dev" || subCmd == "d")
        execute_cmd_net_devices(tokens);
    else if (subCmd == "add" || subCmd == "a")
        execute_cmd_net_add(tokens);
    else if (subCmd == "remove" || subCmd == "delete" || subCmd == "rm")
        execute_cmd_net_remove(tokens);
    else if (subCmd == "status" || subCmd == "st")
        execute_cmd_net_status(tokens);
    else if (subCmd == "exists" || subCmd == "e")
        execute_cmd_net_exists(tokens);
    else if (subCmd == "lastpacket" || subCmd == "lp")
        execute_cmd_net_lastpacket(tokens);
    else if (subCmd == "send" || subCmd == "s")
        execute_cmd_net_send(tokens);
    else if (subCmd == "queue" || subCmd == "q")
        execute_cmd_net_queue(tokens);
    else
        print_error("Unknown network subcommand: " + subCmd);
}

void ConsoleMgr::execute_cmd_net_add(std::vector<std::string> const& tokens) {
    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    const std::string name    = get_token(tokens, 2);
    const std::string portStr = get_token(tokens, 3);
    if (name.empty() || portStr.empty()) {
        print_error("Usage: network add <name> <port> [ip] [pkgsize]");
        return;
    }

    unsigned short port;
    try { port = static_cast<unsigned short>(std::stoi(portStr)); }
    catch (...) { print_error("Invalid port, must be numeric."); return; }

    const std::string ip = get_token(tokens, 4);

    unsigned int pkgSize = 0;
    const std::string pkgSizeStr = get_token(tokens, 5);
    if (!pkgSizeStr.empty()) {
        try { pkgSize = static_cast<unsigned int>(std::stoul(pkgSizeStr)); }
        catch (...) { print_error("Invalid packet size, must be numeric."); return; }
    }

    if (net->addUdpSocket(name, port, ip, pkgSize))
        print("UDP socket '" + name + "' added on port " + portStr + ".");
    else
        print_error("Cannot add UDP socket '" + name + "'.");
}

void ConsoleMgr::execute_cmd_net_remove(std::vector<std::string> const& tokens) {
    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    const std::string name = get_token(tokens, 2);
    if (name.empty()) {
        print_error("Usage: network remove <name>");
        return;
    }

    if (net->removeUdpSocket(name))
        print("UDP socket '" + name + "' removed.");
    else
        print_error("Cannot remove UDP socket '" + name + "'.");
}

void ConsoleMgr::execute_cmd_net_devices(std::vector<std::string> const& tokens){
    const std::string subCmd = get_token(tokens, 2);

    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    if (subCmd == "help") {
        print("Usage: network devices\n"
              "  Lists the active UDP sockets registered in the network module.\n");
        return;
    }

    auto sockets = net->getUdpSockets();
    if (sockets.empty()) {
        print("No UDP sockets registered.");
        return;
    }

    for (UdpSocket* sock : sockets) {
        if (!sock) continue;
        print(sock->name() + " : " + std::to_string(sock->port())
            + (sock->isRunning() ? " (running)" : " (stopped)"));
    }
}

void ConsoleMgr::execute_cmd_net_status(std::vector<std::string> const& tokens) {
    const std::string subCmd = get_token(tokens, 2);

    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    if (subCmd == "help") {
        print("Usage: network status\n"
              "  Shows whether the network module is currently running.\n");
        return;
    }

    print(std::string("Network running: ") + (net->hasSocketsRunnning() ? "YES" : "NO"));
}

void ConsoleMgr::execute_cmd_net_exists(std::vector<std::string> const& tokens) {
    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    const std::string name = get_token(tokens, 2);
    if (name.empty() || name == "help") {
        print("Usage: network exists <name>\n"
              "  Checks whether a UDP socket with that name exists.\n");
        return;
    }

    print("Socket '" + name + "' exists: " + (net->socketExists(name) ? "YES" : "NO"));
}

void ConsoleMgr::execute_cmd_net_lastpacket(std::vector<std::string> const& tokens) {
    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    const std::string name = get_token(tokens, 2);
    if (name.empty() || name == "help") {
        print("Usage: network lastpacket <name>\n"
              "  Shows how long ago (ms) the last packet was received on that socket.\n");
        return;
    }

    if (!net->socketExists(name)) {
        print_error("Socket '" + name + "' not found.");
        return;
    }

    print("Last packet on '" + name + "': " + std::to_string(net->getLastPacketMs(name)) + " ms ago");
}

void ConsoleMgr::execute_cmd_net_send(std::vector<std::string> const& tokens) {
    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    const std::string name = get_token(tokens, 2);
    if (name.empty() || name == "help") {
        print("Usage: network send <name> <ip> <port> <text>\n"
              "  Sends <text> as raw bytes through the UDP socket <name> to <ip>:<port>.\n");
        return;
    }

    const std::string ip      = get_token(tokens, 3);
    const std::string portStr = get_token(tokens, 4);
    const std::string text    = get_token(tokens, 5);

    if (ip.empty() || portStr.empty() || text.empty()) {
        print_error("Usage: network send <name> <ip> <port> <text>");
        return;
    }

    unsigned short port;
    try { port = static_cast<unsigned short>(std::stoi(portStr)); }
    catch (...) { print_error("Invalid port, must be numeric."); return; }

    if (!net->socketExists(name)) {
        print_error("Socket '" + name + "' not found.");
        return;
    }

    const std::vector<char> data(text.begin(), text.end());
    if (net->sendData(name, data, ip, port))
        print("Sent " + std::to_string(data.size()) + " bytes to " + ip + ":" + portStr + ".");
    else
        print_error("Cannot send data through socket '" + name + "'.");
}

void ConsoleMgr::execute_cmd_net_queue(std::vector<std::string> const& tokens) {
    const std::string subCmd = get_token(tokens, 2);

    NetMgr* net = ctrl_ ? ctrl_->getNetModule() : nullptr;
    if (!net) {
        print_error("Network module is not available (null)");
        return;
    }

    if (subCmd == "help") {
        print("Usage: network queue\n"
              "  Shows how many UDP packets are pending in the centralized receive queue.\n");
        return;
    }

    print("Packets pending in queue: " + std::to_string(net->numUdpRcvElements()));
}


// Subcomandos Totalmix -----------------------------------------------------------------

void ConsoleMgr::execute_totalmix_command(std::vector<std::string> const& tokens) {

    if (tokens.size() <= 1 || tokens[1] == "help") {
        print("Available 'totalmix' commands:\n"
              "  help                   - Show this help message\n"
              "  status                 - Show module status\n"
              "  volume <out> <value>   - Set output volume (0-100)\n"
              "  threshold <in> <value> - Set threshold value (0-100)\n"
              "  mute <out>             - Mute an output\n"
              "  unmute <out>           - Unmute an output\n\n"
              "Aliases: totalmix, tmx");
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


// Subcomandos Symetrix -----------------------------------------------------------------

void ConsoleMgr::execute_symetrix_command(std::vector<std::string> const& tokens) {

    const std::string action = get_token(tokens, 1);

    if (action.empty() || action == "help") {
        print("Available 'symetrix' commands:\n"
              "  status                                 - Show connection status\n"
              "  preset <1-1000>                        - Load a Composer preset\n"
              "  set <id> <value> <min> <max> [db]      - Set a component value (curve, or [db] for direct dB)\n"
              "  button <id> <on|off>                   - Set a binary component (button/mute)\n"
              "  supermatrix <in> <out> <volume> [db]   - Set a supermatrix crosspoint volume\n"
              "  tolerance <pct> [id]                   - Update send tolerance (global, or per id)");
        return;
    }

    // Obtener el módulo de symetrix con guard de nulidad
    Symetrix* sym = ctrl_ ? ctrl_->getSymetrixModule() : nullptr;

    if (action == "status") {
        const std::string connStr = (sym && sym->isConnected()) ? "YES" : "NO";
        print("Symetrix connected: " + connStr);
        return;
    }

    if (!sym || !sym->isInitialized()) {
        print_error("Symetrix module not initialized or unavailable");
        return;
    }

    if (action == "preset") {
        const std::string presetStr = get_token(tokens, 2);
        if (presetStr.empty()) { print_error("Usage: symetrix preset <1-1000>"); return; }

        try {
            if (!sym->LoadPreset(static_cast<unsigned int>(std::stoul(presetStr))))
                print_error("Cannot load preset " + presetStr);
        } catch (std::exception const&) {
            print_error("Invalid <preset>, must be numeric.");
        }
        return;
    }

    if (action == "set") {
        const std::string idStr  = get_token(tokens, 2);
        const std::string valStr = get_token(tokens, 3);
        const std::string minStr = get_token(tokens, 4);
        const std::string maxStr = get_token(tokens, 5);
        const bool         useDb = (get_token(tokens, 6) == "db");

        if (idStr.empty() || valStr.empty() || minStr.empty() || maxStr.empty()) {
            print_error("Usage: symetrix set <id> <value> <min> <max> [db]");
            return;
        }

        try {
            const unsigned short id    = static_cast<unsigned short>(std::stoi(idStr));
            const float          value = std::stof(valStr);
            const float          minV  = std::stof(minStr);
            const float          maxV  = std::stof(maxStr);

            const bool ok = useDb
                ? sym->setValue_dB(id, value, minV, maxV)
                : sym->setValue(id, value, minV, maxV);

            if (!ok) SYS_WARN("CLI", "Failed to set Symetrix value.");
        } catch (std::exception const&) {
            print_error("Invalid numeric argument.");
        }
        return;
    }

    if (action == "button") {
        const std::string idStr    = get_token(tokens, 2);
        const std::string stateStr = get_token(tokens, 3);

        if (idStr.empty() || stateStr.empty()) {
            print_error("Usage: symetrix button <id> <on|off>");
            return;
        }

        try {
            const unsigned short id = static_cast<unsigned short>(std::stoi(idStr));
            const bool on = (stateStr == "on" || stateStr == "true" || stateStr == "1");
            if (!sym->setButton(id, on))
                SYS_WARN("CLI", "Failed to set Symetrix button.");
        } catch (std::exception const&) {
            print_error("Invalid <id>, must be numeric.");
        }
        return;
    }

    if (action == "supermatrix") {
        const std::string inStr  = get_token(tokens, 2);
        const std::string outStr = get_token(tokens, 3);
        const std::string volStr = get_token(tokens, 4);
        const bool         useDb = (get_token(tokens, 5) == "db");

        if (inStr.empty() || outStr.empty() || volStr.empty()) {
            print_error("Usage: symetrix supermatrix <in> <out> <volume> [db]");
            return;
        }

        try {
            const unsigned int in     = static_cast<unsigned int>(std::stoul(inStr));
            const unsigned int out    = static_cast<unsigned int>(std::stoul(outStr));
            const float        volume = std::stof(volStr);
            if (!sym->setSupermatrixValue(in, out, volume, useDb))
                SYS_WARN("CLI", "Failed to set Symetrix supermatrix value.");
        } catch (std::exception const&) {
            print_error("Invalid numeric argument.");
        }
        return;
    }

    if (action == "tolerance") {
        const std::string pctStr = get_token(tokens, 2);
        const std::string idStr  = get_token(tokens, 3);

        if (pctStr.empty()) {
            print_error("Usage: symetrix tolerance <pct> [id]");
            return;
        }

        try {
            const unsigned char  pct = static_cast<unsigned char>(std::stoi(pctStr));
            const unsigned short id  = idStr.empty() ? static_cast<unsigned short>(0) : static_cast<unsigned short>(std::stoi(idStr));
            sym->updateTolerancePct_CSQ(pct, id);
        } catch (std::exception const&) {
            print_error("Invalid numeric argument.");
        }
        return;
    }

    print_error("Unknown symetrix subcommand: " + action);
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

std::string ConsoleMgr::get_token(std::vector<std::string> const& tokens, size_t idx) {
    return (idx < tokens.size()) ? tokens[idx] : "";
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
