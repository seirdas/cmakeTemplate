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
     * @brief Genera un mensaje de información en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void info(std::string const& module, std::string const& msg);

    /**
     * @brief Genera un mensaje de warning en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void warning(std::string const& module, std::string const& msg);

    /**
     * @brief Genera un mensaje de error en cout, log.
     * @warning Bloqueante, con pop-up en windows y linux (con zenity)
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void error(std::string const& module, std::string const& msg);
    
    /**
     * @brief Genera un mensaje de solved (solucionado) en cout, log.
     * @param module Módulo (Clase) desde donde se genera el error
     * @param msg Mensaje de error
     */
    void solved(std::string const& module, std::string const& msg);


// Escritura en consola -----------------------------------------------------------------

    /**
     * @brief Activa o desactiva la captura visual del CLI
     */
    void setCliActive(bool active);

    /**
     * @brief Actualiza el texto que el usuario lleva escrito y redibuja la línea
     * @param input Texto completo actual del comando
     * @param cursorPos Posición del cursor dentro de @p input. Por defecto
     *  (std::string::npos) se coloca al final del texto.
     * @param showAppName Indica si debe mostrar el nombre de la app o no
     * @param color Código ANSI de color para el prefijo
     */
    void updateCliInput(
        const std::string& input, 
        size_t cursorPos = std::string::npos,
        bool                showAppName = true,
        std::string         color = ""
    );

    /**
     * @brief Redibuja el prefijo de escritura a la fuerza
     */
    void redrawPrompt();


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
    void show_popup(std::string const& msg, std::string const& title, bool bloq = true);


// Redraw privado -----------------------------------------------------------------------

    /**
     * @brief Dibuja el prompt asumiendo que console_mtx ya fue adquirido
     */
    void redraw_prompt_unlocked();


/************ Variables ****************************************************************/

// Datos de aplicación
    std::string app_name_;      ///< Nombre de aplicación
    bool        initialized_;   ///< Bandera para indicar inicialización exitosa

// Logs de aplicación
    LogMgr      log_;           ///< Log de ejecución de aplicación
    LogMgr      errlog_;        ///< Log de errores de las ejecuciones (persistente)

// Sincronización de mensajes
    std::mutex          console_mtx;        ///< Mutex para evitar solapamientos de mensajes en paralelo

// Parámetros
    const unsigned int  split_width_;       ///< Ancho de separación entre el tipo y el texto
    bool                is_cli_active_;     ///< La terminal está activa para escritura de comandos
    std::string         current_cli_input_; ///< Entrada del comando temporal para no mezclar con logs
    size_t              current_cli_cursor_; ///< Posición del cursor dentro de current_cli_input_

// Consola con entrada activa
    bool cli_show_app_name_;                ///< Mostrar nombre en el prefijo de CLI         
    std::string cli_prefix_color_;          ///< Color ANSI para el prefijo de CLI

};
