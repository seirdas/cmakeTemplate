#pragma once

#include <vector>


// Forward declaration
class Position;         ///< Declaración implícita
class IAppControl;      ///< Declaración implícita


/** 
 * @class CommsDispatcher
 * @brief Clase de gestión de datos recibidos 
 *  externamente de comunicaciones
 */
class CommsDispatcher {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    CommsDispatcher();

    /**
     * @brief Destructor estándar
     */
    ~CommsDispatcher();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa los datos de comunicaciones, incluyendo personas, radios, etc.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
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
     * @brief Cierra la lógica de comunicaciones
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close();

    /**
     * @brief Establece el controlador de la aplicación (ctrl)
     * @param ctrl Controlador de la aplicación
     * @return @c true si el controlador se ha establecido correctamente, @c false en caso contrario
     */
    bool setController(IAppControl* controller);


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Ejecuta la lógica de comunicaciones a partir de un paquete de datos
     *  Normalmente esto es una estructura de interfaz (ICD) obtenida del socket 
     * @param data Estructura de datos de comunicaciones (normalmente del ICD)
     */
    bool Dispatch(std::vector<char> data);


private:

/************ Variables ********************************************************/

// Inicialización y ejecución
    bool            initialized_;       ///< Bandera para indicar inicialización exitosa

// Conexión con AppController (y módulos)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    unsigned long   last_packet_hash_;  ///< Hash del último data recibido, para comparar duplicados

};
