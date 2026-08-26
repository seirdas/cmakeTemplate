#pragma once


// Foward declaration
class IAppControl;


/** 
 * @class TonesDispatcher
 * @brief Clase de gestión de datos recibidos 
 *  externamente de reproducción de tonos
 */
class TonesDispatcher {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    TonesDispatcher();
    
    /**
     * @brief Destructor
     */
    ~TonesDispatcher();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa la lógica de reproducción de tonos
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true si la inicialización fue exitosa, @c false si hubo algún error 
     */
    bool init(void* config);
    
    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);

    /**
     * @brief Cierra la lógica y libera los recursos asociados.
     * @return @c true Si ha cerrado correctamente, @c false en caso de error
     */
    bool close();

    /**
     * @brief Establece el controlador de la aplicación (ctrl)
     * @param ctrl Controlador de la aplicación
     * @return @c true si el controlador se ha establecido correctamente, @c false en caso contrario
     */
    bool setController(IAppControl* controller);


private:

/************ Variables ********************************************************/

// Inicialización
    bool        initialized_;   ///< Bandera para indicar inicialización exitosa

// Conexión con AppController (y módulos)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    unsigned long   last_packet_hash_;  ///< Hash del último data recibido, para comparar duplicados

};
