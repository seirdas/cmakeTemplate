#include "system/sys.hpp"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
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


void Sys::error(std::string const& msg, std::string const& whereIsFrom) {
    log_.write("[ERROR]  "+msg);
    std::cerr << "[ERROR] " << msg << std::endl;
    showPopup(msg, "ERROR");            // <- !! Bloqueante
}

void Sys::warning(std::string const& msg, std::string const& whereIsFrom) {
    log_.write("[WARN]  "+msg);
    std::cerr << "[WARN] " << msg << std::endl;
}

void Sys::info(std::string const& msg, std::string const& whereIsFrom) {
    log_.write("[INFO]  "+msg);
    std::cout << "[INFO] " << msg << std::endl;
}


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