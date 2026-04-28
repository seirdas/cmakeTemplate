#pragma once

#include <string>
#include "system/LogMgr.hpp"

#define SYS_ERROR(module, msg) Sys::instance().error(module, msg)
#define SYS_WARN(module, msg)  Sys::instance().warning(module, msg)
#define SYS_INFO(module, msg)  Sys::instance().info(module, msg)

// singleton
class Sys {
public:
    static Sys& instance();

    void error(std::string const& module, std::string const& msg);
    void warning(std::string const& module, std::string const& msg);
    void info(std::string const& module, std::string const& msg);


// Conversiones -------------------------------------------------------------------------

    /**
     * @brief Convierte std::string a std::wstring
     */
    inline std::wstring stringToWString(std::string const& str);

    /**
     * @brief Convierte std::wstring a std::string 
     */
    inline std::string wstringToString(std::wstring const& ws);

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