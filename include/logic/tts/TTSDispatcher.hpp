#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>


// Foward declaration
class IAppControl;


/** 
 * @class TTSDispatcher
 * @brief Clase de lógica de reproducción de tonos a partir de un paquete
 *  de datos externo (de red, o de servidor iComm)
 */
class TTSDispatcher {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    TTSDispatcher(IAppControl* ctrl = nullptr);
    
    /**
     * @brief Destructor
     */
    ~TTSDispatcher();

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    TTSDispatcher(TTSDispatcher const&) = delete;
    TTSDispatcher& operator=(TTSDispatcher const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    TTSDispatcher(TTSDispatcher&&) = delete;
    TTSDispatcher& operator=(TTSDispatcher&&) = delete;


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


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Toma un paquete de datos de TTS y lo gestiona para reproducirlo
     * @details Esta función es llamada externamente por quien le mande el paquete.
     *  (Habitualmente por el iComm) 
     * @return 
     */
    bool Dispatch();


private:

// Hilos --------------------------------------------------------------------------------

    /**
     * @brief Hilo consumidor de paquetes TTS
     */
    void t_data_consumer();


/************ Variables ********************************************************/

// Pointer to implementation (PIMPL) para añadir iComm (clase administrada CLI.NET)
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

// Inicialización
    std::atomic<bool>   running_;       ///< flag de aplicación corriendo (para hilos)
    bool                initialized_;   ///< Bandera para indicar inicialización exitosa

// Cola de datos (WIP)
    std::thread                 dataConsumer_thread_;   ///< Hilo consumidor de datos TTS (queue)
    std::queue<int>             queue_;                 ///< Cola de comandos
    std::mutex                  queue_mtx_;             ///< Mutex de cola de comandos
    std::condition_variable     queue_cv_;              ///< Conditional variable para mutex de cola

// Conexión con AppController (y módulos)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    unsigned long   last_packet_hash_;  ///< Hash del último data recibido, para comparar duplicados

};
