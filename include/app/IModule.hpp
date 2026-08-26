

/**
 * @brief Agrupa las funciones comunes entre todos los módulos
 * 
 */
class IModule {

public: 

// General ------------------------------------------------------------------------------

    // (Constructor)

    /**
     * @brief Destructor de la interfaz
     */
    virtual ~IModule();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicialización del módulo.
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    virtual bool init(void* config) = 0;

    /**
     * @brief Devuelve si la inicialización ha sido exitosa.
     * @return @c true Si ha iniciado bien, @c false en caso contrario.
     */
    virtual bool isInitialized() const = 0;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    virtual void loadConfig(void* config) = 0;

    /**
    * @brief Para el motor de audio.
    * @returns @c true si la parada ha sido correcta, @c false en caso contrario. 
    */
    virtual bool close() = 0;

};
