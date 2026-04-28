#pragma once

#include <string>
#include "system/LogMgr.hpp"


// Macros simplificados para escribir mensajes:
#define SYS_ERROR(module, msg) Sys::instance().error(module, msg)
#define SYS_WARN(module, msg)  Sys::instance().warning(module, msg)
#define SYS_INFO(module, msg)  Sys::instance().info(module, msg)


/**
 * @class Sys
 * @brief Clase Singleton, para obtener/modificar configuraciones de aplicación
 *      escribir en registro, etc..
 */
class Sys {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Declara la instancia estática de la clase
     */
    static Sys& instance();


// Log ----------------------------------------------------------------------------------

    /**
     * @brief Genera un mensaje de error en cout, log.
     * @warning Bloqueante, con pop-up en windows y linux (con zenity)
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void error(std::string const& module, std::string const& msg);

    /**
     * @brief Genera un mensaje de warning en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void warning(std::string const& module, std::string const& msg);

    /**
     * @brief Genera un mensaje de información en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
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

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor privado en singleton
     */
    Sys();

    /**
     * @brief Destructor privado en singleton 
     */
    ~Sys();

    /**
     * @brief Evitar copias
     */
    Sys(const Sys&)             = delete;
    Sys& operator=(const Sys&)  = delete;


// Pop-ups ------------------------------------------------------------------------------

    /** 
     * @brief Muestra una ventana de error según el SO
     */
    void showPopup(const std::string& msg, const std::string& title);


/************ Variables ****************************************************************/

    LogMgr log_;    // Clase para gestión de archivo de log interno
};
