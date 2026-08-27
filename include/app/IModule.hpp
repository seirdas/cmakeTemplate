#include <atomic>


// Forward declaration
class IAppControl;


/**
 * @brief Agrupa las funciones comunes entre todos los módulos
 */
class IModule {

public: 

// General ------------------------------------------------------------------------------

    // (Constructor)
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


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicialización del módulo.
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config);

    /**
     * @brief Parte de inicialización particular del módulo heredado.
     * @note Este método es el que debe sobreescribirse en las clases heredadas
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    virtual bool onInit();

    /**
     * @brief Devuelve si la inicialización ha sido exitosa.
     * @return @c true Si ha iniciado bien, @c false en caso contrario.
     */
    virtual bool isInitialized() const;


// Configuración ------------------------------------------------------------------------

    /**
    * @brief Carga y valida la configuración de la aplicación desde una configuración.
    *  Diseñado para recibir un puntero a json.
    * @param config Puntero al JSON que contiene los parámetros de configuración.
    */
    virtual void loadConfig(void* config) = 0;


// Cierre -------------------------------------------------------------------------------

    /**
    * @brief Cierra el módulo
    * @returns @c true si se ha cerrado correctamente, @c false en caso contrario. 
    */
    bool close();

    /**
    * @brief Parte de cerrar particular del módulo heredado.
     * @note Este método es el que debe sobreescribirse en las clases heredadas
    * @returns @c true si se ha cerrado correctamente, @c false en caso contrario. 
    */
    virtual bool onClose();


// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Establece el controlador de la aplicación (ctrl)
     * @param ctrl Controlador de la aplicación
     * @return @c true si el controlador se ha establecido correctamente, @c false en caso contrario
     */
    bool setController(IAppControl* controller);


protected:

// Inicialización y ejecución
    IAppControl*        ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros    
    void*               config_;            ///< Configuración del módulo (considerado json)
    bool                initialized_;       ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>   threads_running_;   ///< Mantiene los hilos del módulo corriendo

};
