#pragma once

#include <string>
#include "system/LogMgr.hpp"

#define SYS_ERROR(msg, module) Sys::instance().error(msg, module)
#define SYS_WARN(msg, module)  Sys::instance().warning(msg, module)
#define SYS_INFO(msg, module)  Sys::instance().info(msg, module)

// singleton
class Sys {
public:
    static Sys& instance();

    void error(std::string const& msg, std::string const& whereIsFrom);
    void warning(std::string const& msg, std::string const& whereIsFrom);
    void info(std::string const& msg, std::string const& whereIsFrom);

private:
    Sys(); // constructor privado
    ~Sys();

    // Evitar copias
    Sys(const Sys&) = delete;
    Sys& operator=(const Sys&) = delete;

    void showPopup(const std::string& msg, const std::string& title);

private:
    LogMgr log_;
};