#include "system/SystemMgr.hpp"
#include "system/ANSI.hpp"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <thread>
#elif __linux__
    #include <unistd.h> // UNIX standard
    #include <cwchar> 
    #include <clocale>
#endif


// General ------------------------------------------------------------------------------

SystemMgr& SystemMgr::instance() {
    static SystemMgr instance;
    return instance;
}


// Datos de aplicación ------------------------------------------------------------------

    /**
     * @brief Establece el nombre de la aplicación (para el log)
     * @param name Nombre de aplicación
     */
    void SystemMgr::setAppName(const std::string& name) {
        app_name_ = name;
    }

    /**
     * @brief Devuelve el nombre de la aplicación
     * @return Nombre de aplicación
     */
    std::string const& SystemMgr::getAppName() const {
        return app_name_;
    }


// Inicialización y ejecución -----------------------------------------------------------

bool SystemMgr::init(const std::string& appName) {

    // Si se inicializa con nombre, utiliza ese para el nombre de la app
    if (!appName.empty())
        app_name_ = appName;

    // Crear el archivo de log con el nombre del ejecutable
    log_.init(app_name_ + ".log");
    errlog_.init("errors.log");

    // Se limpia solamente el log de ejecución
    log_.clear();

    // indica nueva ejecución en log de errores
    errlog_.write("-- init --");
    
    initialized_ = true;
    return initialized_; // <- true
}

bool SystemMgr::isInitialized() const {
    return initialized_;
}


// Log ----------------------------------------------------------------------------------

void SystemMgr::info(std::string const& module, std::string const& msg) {
    std::string prefix = "[INFO]   ";
    std::string module_brackets = "[" + module + "]";

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(split_width_) << module_brackets << msg;
    log_.write(ss.str());
    
    // Protección para la consola
    std::lock_guard<std::mutex> lock(console_mtx);

    // Borra el prefijo de comandos
    if (is_cli_active_)
        std::cerr << "\r\033[2K";

    // Escribir el mensaje en consola (si aplica)
    std::cerr << ss.str() << std::endl;

    // Poner otra vez el prefijo de comandos
    if (is_cli_active_)
        redrawPrompt_unlocked();

}

void SystemMgr::warning(std::string const& module, std::string const& msg) {
    std::string prefix = "[WARN]   ";
    std::string module_brackets = "[" + module + "]";

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(split_width_) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Protección para la consola
    std::lock_guard<std::mutex> lock(console_mtx);

    // Borra el prefijo de comandos
    if (is_cli_active_)
        std::cerr << "\r\033[2K";

    // Escribir el mensaje
    std::cerr << ANSI_YELLOW << ss.str() << ANSI_RESET << std::endl;

    // Poner otra vez el prefijo de comandos
    if (is_cli_active_)
        redrawPrompt_unlocked();
}

void SystemMgr::error(std::string const& module, std::string const& msg) {
    std::string prefix = "[ERROR]   ";
    std::string module_brackets = "[" + module + "]";

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(split_width_) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Protección para la consola
    std::unique_lock<std::mutex> lock(console_mtx);

    // Borra el prefijo de comandos
    if (is_cli_active_)
        std::cerr << "\r\033[2K";

    // Escribir el mensaje
    std::cerr << ANSI_RED << ss.str() << ANSI_RESET << std::endl;

    // Poner otra vez el prefijo de comandos
    if (is_cli_active_)
        redrawPrompt_unlocked();

    // Liberar el mutex para el popup
    lock.unlock();

    // Mostrar también ventana de error
    show_popup(msg, app_name_);            // <- !! Bloqueante
}

void SystemMgr::solved(std::string const& module, std::string const& msg) {
    std::string prefix = "[SOLV]   ";
    std::string module_brackets = "[" + module + "]";

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(split_width_) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Protección para la consola
    std::lock_guard<std::mutex> lock(console_mtx);

    // Borra el prefijo de comandos
    if (is_cli_active_)
        std::cerr << "\r\033[2K";

    // Escribir el mensaje
    std::cout << ANSI_GREEN << ss.str() << ANSI_RESET << std::endl;

    // Poner otra vez el prefijo de comandos
    if (is_cli_active_)
        redrawPrompt_unlocked();
}


// Escritura en consola -----------------------------------------------------------------

void SystemMgr::setCliActive(bool active) {
    std::lock_guard<std::mutex> lock(console_mtx);
    is_cli_active_ = active;
}

void SystemMgr::updateCliInput(const std::string& input, size_t cursorPos) {
    std::lock_guard<std::mutex> lock(console_mtx);
    current_cli_input_  = input;
    current_cli_cursor_ = (cursorPos == std::string::npos) ? input.size() : cursorPos;
    if (is_cli_active_)
        redrawPrompt_unlocked();
}

void SystemMgr::redrawPrompt() {
    std::lock_guard<std::mutex> lock(console_mtx);
    redrawPrompt_unlocked();
}

// General ------------------------------------------------------------------------------

SystemMgr::SystemMgr() :
    split_width_(20),
    is_cli_active_(false),
    current_cli_cursor_(0)
{

}

SystemMgr::~SystemMgr() {
    
}


// Pop-ups ------------------------------------------------------------------------------

void SystemMgr::show_popup(std::string const& msg, std::string const& title, bool bloq) {
    #ifdef _WIN32

        if(bloq)
            MessageBoxA(NULL, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
        else {
            std::thread([msg, title]() {
                MessageBoxA(NULL, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
            }).detach();
        }
    #else
        pid_t pid = fork();

        if (pid == 0) {
            execlp("zenity",
                "zenity",
                "--error",
                "--title", title.c_str(),
                "--text", msg.c_str(),
                (char*)NULL);
            _exit(1); // si falla
        }
    #endif
}


// Redraw privado -----------------------------------------------------------------------

void SystemMgr::redrawPrompt_unlocked() {
    // \r                   -> Mueve el cursor al inicio de la línea
    // ANSI_CLEAR_TO_EOL    -> Borra únicamente desde el cursor hasta el final
    std::cerr << "\r" << ANSI_BRIGHT_CYAN << app_name_ << "> " << ANSI_RESET
              << current_cli_input_ << ANSI_RESET << ANSI_CLEAR_TO_EOL;

    // Si el cursor lógico no está al final del texto, hay que retroceder el
    // cursor visual lo que sobre (siempre se pinta la línea entera arriba).
    const size_t charsAfterCursor = current_cli_input_.size() - current_cli_cursor_;
    if (charsAfterCursor > 0) {
        std::cerr << "\033[" << charsAfterCursor << "D";
    }

    std::cerr << std::flush;
}