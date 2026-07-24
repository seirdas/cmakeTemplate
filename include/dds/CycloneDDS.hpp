#pragma once


#include <memory>
#include <thread>

class HelloWorld;

/**
 * @class CycloneDDS
 * @brief Gestor de comunicaciones basado en el protocolo Data Distribution Service (DDS) mediante CycloneDDS.
 * 
 * Esta clase abstrae la lógica de publicación y suscripción de tópicos utilizando el middleware
 * CycloneDDS. Permite el intercambio de datos a alta velocidad y en tiempo real con otros nodos
 * de la red de forma desacoplada.
 */
class CycloneDDS {

public:

// General ------------------------------------------------------------------------------
    
    /**
     * @brief Constructor estándar 
     */
    CycloneDDS();

    /**
     * @brief Destructor estándar 
     */
    ~CycloneDDS();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicialización del servidor CycloeDDS
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
     * @brief Cierra y limpia todos los componentes de la clase.
     * @details Esto incluye los datos PIMPL: Participant, publisher, subscriber, etc. 
     */
    void close();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Ejemplo de uso de CycloneDDS 
     */
    void test();

    /**
     * @brief Publica datos del idl en un intervalo de tiempo
     */
    void run_publisher();

    /**
     * @brief Publica datos del idl en el momento que se llama
     */
    bool publish_now(HelloWorld& msg);

    /**
     * @brief Recibe datos del idl 
     * @warning BLOQUEANTE 
     */
    void run_subscriber();

    /**
     * @brief Detiene el servicio de subscripción y deja de recibir datos 
     */
    void stop_subscriber();


private:

/************ Variables ****************************************************************/


// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;                        ///< Declaración de estructura PIMPL para no depender de la librería en el header
    std::unique_ptr<Impl>   pimpl_;     ///< Miembros dependientes de la librería externa

// Inicialización
    bool                initialized_;   ///< Bandera para indicar inicialización exitosa
    bool                enabled_;       ///< Activa la comunicación DDS a través de CycloneDDS

// Datos de CycloneDDS
    unsigned int        DOMAIN_ID_;
    std::string         pqos_name_;

// Suscriptor
    std::thread         sub_thread_;
    std::atomic<bool>   sub_running_;

};
