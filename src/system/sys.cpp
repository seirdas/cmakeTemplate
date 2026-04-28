#include "system/sys.hpp"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h> // UNIX standard
    #include <cwchar> 
    #include <clocale>
#endif


Sys& Sys::instance() {
    static Sys instance;
    return instance;
}

Sys::Sys() :
    log_("system.log")
{}

Sys::~Sys() {
    
}


// Log ----------------------------------------------------------------------------------

void Sys::error(std::string const& module, std::string const& msg) {
    std::string prefix = "[ERROR]   ";

    log_.write(prefix+"["+module+"] "+msg);
    std::cerr << prefix << "[" << module << "]  " <<" "<< msg << std::endl;
    showPopup(msg, "ERROR");            // <- !! Bloqueante
}

void Sys::warning(std::string const& module, std::string const& msg) {
    std::string prefix = "[WARN]   ";

    log_.write(prefix+"["+module+"] "+msg);
    std::cerr << prefix << "[" << module << "]  " <<" "<< msg << std::endl;
}

void Sys::info(std::string const& module, std::string const& msg) {
    std::string prefix = "[INFO]   ";

    log_.write(prefix+"["+module+"] "+msg);
    std::cout << prefix << "[" << module << "]  " <<" "<< msg << std::endl;
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

void Sys::showPopup(const std::string& msg, const std::string& title) {
    #ifdef _WIN32
        MessageBoxA(NULL, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
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
