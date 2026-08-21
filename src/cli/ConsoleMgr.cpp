#include "cli/ConsoleMgr.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"

// Módulos
#include "app/IAppControl.hpp"
#include "sound/SoundMgr.hpp"
#include "sound/PlayerAudio.hpp"
#include "sound/PlayerMorse.hpp"
#include "sound/PlayerTTS.hpp"
#include "devices/TotalMix.hpp"
#include "devices/Symetrix.hpp"

// Otros
#include <iostream>
#include <functional>


// General ------------------------------------------------------------------------------

ConsoleMgr::ConsoleMgr(IAppControl* ctrl) :
    ctrl_(ctrl),
    initialized_(false),
    running_(false),
    AppName_("app")
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

    // #TODO

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

bool ConsoleMgr::LaunchConsole() {

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

    // Poner el nombre de ventana
    setConsoleTitle(AppName_);
}

void ConsoleMgr::setConsoleTitle(std::string const& title) {
    #ifdef _WIN32
        SetConsoleTitleA(title.c_str());
    #else
        std::cout << "\033]0;" << title << "\007" << std::flush;
    #endif
}


// Comandos -----------------------------------------------------------------------------

void ConsoleMgr::execute_command(std::string const& cmd) {

    // Obtener las palabras por separado
    std::vector<std::string> tokens = tokenize_cli(cmd);

    // Comprobar que hay palabras
    if (tokens.empty())
        return;

    std::string const& name = tokens[0];

    // Switch con strings -> Tabla de dispatch: nombre de comando -> handler(tokens).
    // Para añadir un comando nuevo basta con añadir una entrada aquí.
    using CliHandler = std::function<void(std::vector<std::string> const&)>;
    const std::unordered_map<std::string, CliHandler> handlers = {

        { "help", [](std::vector<std::string> const&) {
            std::cout << "Available commands:\n";
            std::cout << "  help                - Show this help message\n";
            std::cout << "  status              - Show app general status\n";
            std::cout << "  sounds [args]       - Execute sound-related commands (also works with 'sound')\n";
            std::cout << "  symetrix [args]     - Execute symetrix-related commands\n";
            std::cout << "  totalmix [args]     - Execute totalmix-related commands\n";
            std::cout << "  exit                - Exit the application\n";
            std::cout << "\n";
            std::cout << "Use '[command] help' for more information about a command\n";
            std::cout << "\n";
        }},

        { "status", [this](std::vector<std::string> const& tokens) {
            
            std::string const sub = (tokens.size() > 1) ? tokens[1] : "help";

            if(tokens.size() == 1)
                SYS_INFO("CLI", "Online mode: " + std::string(ctrl_->isOnlineMode() ? "YES" : "NO"));
            else if(tokens.size() == 2) {
                if (tokens[1] == "help") {
                    std::cout << "  Shows app general status.\n";
                    return;
                }
                else
                    SYS_WARN("CLI", "Unknown subcommand: " + sub);
            }
            else
                SYS_WARN("CLI", "Unknown subcommand: " + sub);
        }},

        { "sounds", [this](std::vector<std::string> const& t) {
            execute_sounds_command(t);
        }},

        { "sound", [this](std::vector<std::string> const& t) {
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
        SYS_WARN("CLI", "Unknown command: " + name);
}

void ConsoleMgr::execute_sounds_command(std::vector<std::string> const& tokens) {

    // tokens[0] == "sounds"; tokens[1] es el subcomando
    std::string const sub = (tokens.size() > 1) ? tokens[1] : "help";

    if (sub == "help") {
        std::cout << "Available 'sounds' commands:\n";
        std::cout << "  sounds devices          - List available playback/capture devices\n";
        std::cout << "  sounds players          - List active audio/morse/tts modules\n";
        std::cout << "  sounds morse \"<text>\" [--player \"<name>\"]\n";
        std::cout << "\n";
        std::cout << "Use 'sounds [command] help' for more information about a command\n";
        std::cout << "\n";
        return;
    }

    // Obtener el módulo de sonidos
    SoundMgr* snd = ctrl_->getSoundsModule();

    if (sub == "devices") {
        std::cout << "Available playback devices:\n";
        for (auto const& name : snd->getAvailablePlaybacks())
            std::cout << "  - " << name << "\n";

        std::cout << "Available capture devices:\n";
        for (auto const& name : snd->getAvailableCaptures())
            std::cout << "  - " << name << "\n";
        return;
    }

    if (sub == "players") {
        std::cout << "Available Player modules:\n";
        std::cout << "\n";
        std::cout << "PlayerAudio modules:\n";
        for (auto const& name : snd->getPlayerAudioNames())
            std::cout << "  - " << name << "\n";

        std::cout << "PlayerMorse modules:\n";
        for (auto const& name : snd->getPlayerMorseNames())
            std::cout << "  - " << name << "\n";

        std::cout << "PlayerTTS modules:\n";
        for (auto const& name : snd->getPlayerTTSNames())
            std::cout << "  - " << name << "\n";
        return;
    }

    if (sub == "morse") {

        // Comprobar que mínimo tiene 3 argumentos (el texto)
        if (tokens.size() < 3) {
            std::cout << "Available 'sound morse' commands:\n";
            std::cout << "  sounds morse \"<text>\" [--player \"<name>\"]\n";
            std::cout << "\n";
            return;
        }

        // Argumento posicional obligatorio: tokens[2] = texto a codificar
        std::string const& text = tokens[2];

        std::unordered_map<std::string, std::string> flags = parse_flags(tokens, 3);
        auto playerIt = flags.find("--player");

        std::string playerName = "";
        if (playerIt != flags.end()) {
            if (playerIt->second.empty())
                SYS_WARN("CLI", "--player requires a value.");
            else
                playerName = playerIt->second;
        }

        PlayerMorse* pm = snd->getPlayerMorse(playerName);
        if (!pm) {
            SYS_WARN("CLI", "Morse module '" + playerName + "' not available.");
            return;
        }

        pm->playMorse(text);
        return;
    }

    SYS_WARN("CLI", "Unknown sounds subcommand: " + sub);
}

void ConsoleMgr::execute_symetrix_command(std::vector<std::string> const& tokens) {

    // tokens[0] == "symetrix"; tokens[1] es el subcomando
    std::string const sub = (tokens.size() > 1) ? tokens[1] : "help";

    if (sub == "help") {
        std::cout << "Available 'symetrix' commands:\n";
        std::cout << "  symetrix status  - Show connection status\n";
        std::cout << "\n";
        return;
    }

    // Obtener el módulo de symetrix
    Symetrix* sym = ctrl_->getSymetrixModule();

    if (sub == "status") {
        SYS_INFO("CLI", std::string("Symetrix connected: ") + (sym->isConnected() ? "YES" : "NO"));
        return;
    }

    SYS_WARN("CLI", "Unknown symetrix subcommand: " + sub);
}

void ConsoleMgr::execute_totalmix_command(std::vector<std::string> const& tokens) {

    // tokens[0] == "totalmix"; tokens[1] es el subcomando
    std::string const sub = (tokens.size() > 1) ? tokens[1] : "help";

    if (sub == "help") {
        std::cout << "Available 'totalmix' commands:\n";
        std::cout << "  totalmix status                 - Show module status\n";
        std::cout << "  totalmix volume <out> <value>   - Set output volume (0-100)\n";
        std::cout << "  totalmix threshold <in> <value> - Set threshold value (0-100)\n";
        std::cout << "  totalmix mute <out>             - Mute an output\n";
        std::cout << "  totalmix unmute <out>           - Unmute an output\n";
        std::cout << "\n";
        return;
    }

    // Obtener el módulo de Totalmix
    TotalMix* tmx = ctrl_->getTotalmixModule();

    if (sub == "status") {
        SYS_INFO("CLI", std::string("Totalmix module initialized: ") + (tmx->isInitialized() ? "YES" : "NO"));
        return;
    }

    if (sub == "volume") {
        // Argumentos posicionales obligatorios: tokens[2] = canal, tokens[3] = valor
        if (tokens.size() < 4) {
            SYS_WARN("CLI", "Usage: totalmix volume <out> <value>");
            return;
        }

        try {
            int   out   = std::stoi(tokens[2]);
            float value = std::stof(tokens[3]);
            if (!tmx->SetOutputVolume(out, value))
                SYS_WARN("CLI", "Failed to set Totalmix output volume.");
        } catch (std::exception const&) {
            SYS_WARN("CLI", "Invalid <out>/<value>, must be numeric.");
        }
        return;
    }

    if (sub == "threshold") {
        // Argumentos posicionales obligatorios: tokens[2] = canal, tokens[3] = valor
        if (tokens.size() < 4) {
            SYS_WARN("CLI", "Usage: totalmix volume <out> <value>");
            return;
        }

        try {
            int   in   = std::stoi(tokens[2]);
            float value = std::stof(tokens[3]);
            if (!tmx->SetInputThreshold(in, value))
                SYS_WARN("CLI", "Failed to set Totalmix output volume.");
        } catch (std::exception const&) {
            SYS_WARN("CLI", "Invalid <in>/<value>, must be numeric.");
        }
        return;
    }

    if (sub == "mute" || sub == "unmute") {
        // Argumento posicional obligatorio: tokens[2] = canal
        if (tokens.size() < 3) {
            SYS_WARN("CLI", "Usage: totalmix " + sub + " <out>");
            return;
        }

        try {
            int out = std::stoi(tokens[2]);
            if (!tmx->SetMuteOutput(out, sub == "mute"))
                SYS_WARN("CLI", "Failed to set Totalmix mute state.");
        } catch (std::exception const&) {
            SYS_WARN("CLI", "Invalid <out>, must be numeric.");
        }
        return;
    }

    SYS_WARN("CLI", "Unknown totalmix subcommand: " + sub);
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
