#pragma once

#include <string>
#include "files/LogMgr.hpp"
#include <mutex>

// Macros simplificados para escribir mensajes:
#define SYS_ERROR(module, msg)  SystemMgr::instance().error(module, msg)
#define SYS_WARN(module, msg)   SystemMgr::instance().warning(module, msg)
#define SYS_INFO(module, msg)   SystemMgr::instance().info(module, msg)
#define SYS_SOLVED(module, msg) SystemMgr::instance().solved(module, msg)


/**
 * @class SystemMgr
 * @brief Clase Singleton, para obtener/modificar configuraciones de aplicación
 *      escribir en registro, etc..
 */
class SystemMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Declara la instancia estática de la clase
     */
    static SystemMgr& instance();

    
// Datos de aplicación ------------------------------------------------------------------

    /**
     * @brief Establece el nombre de la aplicación (para el log)
     * @param name Nombre de aplicación
     */
    void setAppName(const std::string& name);

    /**
     * @brief Devuelve el nombre de la aplicación
     * @return Nombre de aplicación
     */
    std::string const& getAppName() const;


// Inicialización y ejecución -----------------------------------------------------------

    /**
     * @brief Inicializa la clase de gestión de sistema de la aplicación
     *  Utiliza el nombre de la aplicación (app_name_) para crear el log de la app.
     */
    bool init(const std::string& appName = "");

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    
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
    
    /**
     * @brief Genera un mensaje de solved (solucionado) en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void solved(std::string const& module, std::string const& msg);


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
    SystemMgr();

    /**
     * @brief Destructor privado en singleton 
     */
    ~SystemMgr();

    /**
     * @brief Evitar copias
     */
    SystemMgr(const SystemMgr&)             = delete;
    SystemMgr& operator=(const SystemMgr&)  = delete;


// Pop-ups ------------------------------------------------------------------------------

    /** 
     * @brief Muestra una ventana de error según el SO
     */
    void showPopup(std::string const& msg, std::string const& title, bool bloq = true);

    
private:

/************ Variables ****************************************************************/

// Datos de aplicación
    std::string app_name_;      ///< Nombre de aplicación
    bool        initialized_;   ///< Bandera para indicar inicialización exitosa

// Logs de aplicación
    LogMgr log_;             ///< Log de ejecución de aplicación
    LogMgr errlog_;          ///< Log de errores de las ejecuciones (persistente)

// Sincronización de mensajes
    std::mutex  console_mtx;     ///< Mutex para evitar solapamientos de mensajes en paralelo

};
