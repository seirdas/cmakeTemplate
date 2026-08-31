#pragma once

#include <atomic>


// Forward declaration
class IAppControl;


/**
 * @brief Agrupa las funciones comunes entre todos los módulos
 */
class IModule {

public: 

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de la interfaz
     */
    IModule();

    /**
     * @brief Destructor de la interfaz
     */
    virtual ~IModule();

    // Sin copia ni movimiento para interfaces/clases base
    IModule(const IModule&) = delete;
    IModule& operator=(const IModule&) = delete;
    IModule(IModule&&) = delete;
    IModule& operator=(IModule&&) = delete;


// Inicialización y cierre --------------------------------------------------------------

    /**
     * @brief Inicialización del módulo.
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    virtual bool init(void* config = nullptr);

    /**
    * @brief Cierra el módulo
    * @returns @c true si se ha cerrado correctamente, @c false en caso contrario. 
    */
    virtual bool close();

    /**
     * @brief Cierra el módulo y lo abre de nuevo (close()->init())
     * @return @c true si se ha reiniciado correctamente, @c false en caso contrario
     */
    bool reload();


// Configuración ------------------------------------------------------------------------

    /**
    * @brief Carga y valida la configuración de la aplicación desde una configuración.
    *  Diseñado para recibir un puntero a json.
    * @param config Puntero al JSON que contiene los parámetros de configuración.
    */
    virtual void loadConfig(void* config) = 0;


// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Establece el controlador de la aplicación (ctrl)
     * @param ctrl Controlador de la aplicación
     * @return @c true si el controlador se ha establecido correctamente, @c false en caso contrario
     */
    bool setController(IAppControl* controller);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa.
     * @return @c true Si ha iniciado bien, @c false en caso contrario.
     */
    virtual bool isInitialized() const;


protected:

/************ Variables ****************************************************************/

// Inicialización y ejecución
    IAppControl*        ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros    
    void*               config_;            ///< Configuración del módulo (considerado json)
    bool                initialized_;       ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>   threads_running_;   ///< Mantiene los hilos del módulo corriendo
    
};
