#include "system/SystemMgr.hpp"
#include <iostream>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h> // UNIX standard
    #include <cwchar> 
    #include <clocale>
#endif

// APP_NAME debería haber sido inyectado desde CMakeLists
#ifndef APP_NAME
    #define APP_NAME "app"
#endif

// Definición de códigos de escape ANSI para colores
const std::string ANSI_RESET      		= "\033[0m";
const std::string ANSI_BLACK      		= "\033[30m";
const std::string ANSI_RED        		= "\033[31m";
const std::string ANSI_GREEN      		= "\033[32m";
const std::string ANSI_YELLOW     		= "\033[33m";
const std::string ANSI_BLUE       		= "\033[34m";
const std::string ANSI_MAGENTA    		= "\033[35m";
const std::string ANSI_CYAN       		= "\033[36m";
const std::string ANSI_WHITE      		= "\033[37m";
const std::string ANSI_BRIGHT_RED     	= "\033[91m";
const std::string ANSI_BRIGHT_GREEN   	= "\033[92m";
const std::string ANSI_BRIGHT_YELLOW  	= "\033[93m";
const std::string ANSI_BRIGHT_BLUE    	= "\033[94m";
const std::string ANSI_BRIGHT_MAGENTA 	= "\033[95m";
const std::string ANSI_BRIGHT_CYAN    	= "\033[96m";
const std::string ANSI_BOLD           	= "\033[1m";
const std::string ANSI_UNDERLINE      	= "\033[4m";


// General ------------------------------------------------------------------------------

SystemMgr& SystemMgr::instance() {
    static SystemMgr instance;
    return instance;
}

SystemMgr::SystemMgr() :
    log_(std::string(APP_NAME) + (".log")),
    errlog_("errors.log")
{
    // Se limpia solo el log de ejecución
    log_.clear();

    // indica nueva ejecución en log de errores
    errlog_.write("-- init --");
}

SystemMgr::~SystemMgr() {
    
}


// Log ----------------------------------------------------------------------------------

void SystemMgr::error(std::string const& module, std::string const& msg) {
    std::string prefix = "[ERROR]   ";
    std::string module_brackets = "[" + module + "]";
    const int width = 20; // Ancho estandarizado

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(width) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Para la consola
    std::lock_guard<std::mutex> lock(console_mtx);
    std::cerr << ANSI_RED << prefix 
              << std::left << std::setw(width) << module_brackets 
              << msg << ANSI_RESET << std::endl;

    // Mostrar también ventana de error
    showPopup(msg, APP_NAME);            // <- !! Bloqueante
}

void SystemMgr::warning(std::string const& module, std::string const& msg) {
    std::string prefix = "[WARN]   ";
    std::string module_brackets = "[" + module + "]";
    const int width = 20; // Ancho estandarizado

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(width) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Para la consola
    std::lock_guard<std::mutex> lock(console_mtx);
    std::cerr << ANSI_YELLOW << prefix 
              << std::left << std::setw(width) << module_brackets 
              << msg << ANSI_RESET << std::endl;
}

void SystemMgr::info(std::string const& module, std::string const& msg) {
    std::string prefix = "[INFO]   ";
    std::string module_brackets = "[" + module + "]";
    const int width = 20; // Ancho estandarizado

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(width) << module_brackets << msg;
    log_.write(ss.str());
    
    // Para la consola
    std::lock_guard<std::mutex> lock(console_mtx);
    std::cerr << prefix 
              << std::left << std::setw(width) << module_brackets 
              << msg << std::endl;
}

void SystemMgr::solved(std::string const& module, std::string const& msg) {
    std::string prefix = "[SOLV]   ";
    std::string module_brackets = "[" + module + "]";
    const int width = 20; // Ancho estandarizado

    // Para el archivo de log (usando un stringstream para aplicar el ancho)
    std::ostringstream ss;
    ss << prefix << std::left << std::setw(width) << module_brackets << msg;
    log_.write(ss.str());
    errlog_.write(ss.str());
    
    // Para la consola
    std::lock_guard<std::mutex> lock(console_mtx);
    std::cerr << ANSI_GREEN << prefix 
              << std::left << std::setw(width) << module_brackets 
              << msg << ANSI_RESET << std::endl;
}


// Conversiones -------------------------------------------------------------------------

inline std::wstring stringToWString(std::string const& str) {
    if (str.empty()) return {};

    #ifdef _WIN32
        int size = MultiByteToWideChar(CP_UTF8, 0,
                                    str.c_str(), static_cast<int>(str.size()),
                                    nullptr, 0);
        if (size <= 0) return {};

        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0,
                            str.c_str(), static_cast<int>(str.size()),
                            result.data(), size);
        return result;

    #else
        // Asegura que el locale soporte UTF-8
        const char* locale = setlocale(LC_ALL, "");
        (void)locale;

        std::mbstate_t state{};
        const char* src = str.c_str();
        std::size_t size = std::mbsrtowcs(nullptr, &src, 0, &state);
        if (size == static_cast<std::size_t>(-1)) return {};

        std::wstring result(size, L'\0');
        src = str.c_str();  // mbsrtowcs avanza el puntero, hay que resetearlo
        std::mbsrtowcs(result.data(), &src, size, &state);
        return result;
    #endif
}

inline std::string wstringToString(std::wstring const& ws) {
    if (ws.empty()) return "";

    #ifdef _WIN32
        // 1. Obtener el tamaño necesario para el buffer string (ANSI/UTF-8)
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
        
        // 2. Crear el string y convertir
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    #else
        // Versión para Linux/Posix
        std::mbstate_t state = std::mbstate_t();
        const wchar_t* src = ws.c_str();
        
        // 1. Calcular tamaño necesario
        size_t len = std::wcsrtombs(NULL, &src, 0, &state);
        if (len == static_cast<size_t>(-1)) return "";

        // 2. Convertir
        std::string strTo(len, 0);
        std::wcsrtombs(&strTo[0], &src, len, &state);
        return strTo;
    #endif
}


// Pop-ups ------------------------------------------------------------------------------

void SystemMgr::showPopup(std::string const& msg, std::string const& title, bool bloq) {
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
