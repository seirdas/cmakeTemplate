#pragma once

#include <vector>

class Persona;      ///< Declaración implícita
class IAppControl;  ///< Declaración implícita


/** 
 * @class CommsCore
 * @brief Clase de gestión de comunicaciones entre personas
 */
class CommsCore {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    CommsCore(IAppControl* ctrl_ = nullptr);

    /**
     * @brief Destructor estándar
     */
    ~CommsCore();

// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa los datos de comunicaciones, incluyendo personas, radios, etc.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config);


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Ejecuta la lógica de comunicaciones a partir de un paquete de datos
     *  Normalmente esto es una estructura de interfaz (ICD) obtenida del socket 
     * @param data Estructura de datos de comunicaciones (normalmente del ICD)
     */
    bool Ejecutar(std::vector<char> data);


private:

/************ Variables ********************************************************/

// Conexión con AppController (y módulos)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    size_t          last_packet_hash_;  ///< Hash del último data recibido, para comparar duplicados

// Gestión de personas
    std::vector<Persona> personas_;     ///< Lista de personas gestionadas

};
