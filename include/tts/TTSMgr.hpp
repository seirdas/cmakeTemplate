#pragma once

#include <memory>   // unique_ptr
#include <string>
#include <queue>


// intentar quitar esto si es posible:
#include "datatypes/TTSDataTypes.hpp"
#include "sound/TTSCore.hpp"
#include "tts/ITTSObserver.hpp"     // Clase para observadores


class TTSMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSMgr();
    
    /**
     * @brief Destructor 
     */
    ~TTSMgr();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa TTSMgr incluyendo el núcleo de TTS, los 
     *  reproductores de TTS y el cliente de iComm administrado
     * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
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
     * @brief Cierra TTSMgr incluyendo el núcleo de TTS, los
     *  reproductores de TTS y el cliente de iComm administrado
     */
    void close();

    /**
     * @brief Añade un paquete de datos TTS listo para procesar
     *  Particularmente y en resumen, texto que será reproducido (entre otros datos)
     * @param packet Paquete de datos para procesar
     */
    void Ejecutar(const TTSPacket& packet);


// PlaybackTTS ----------------------------------------------------------------------------

    bool play(
        std::string const& text, 
        std::string const& entityName,
        std::string const& modelName = "", 
        std::string const& playbackName = "" );


// Observadores -------------------------------------------------------------------------

    /**
     * @brief Método para añadir una clase observadora
     */
    void addObserver(ITTSObserver* obs);
    

private:

// Notificar a observadores -------------------------------------------------------------

    /**
     * @brief Método privado para encapsular datos y notificarlos externamente
     *  a través del patrón observador
     */
    void notify();


// Hilos --------------------------------------------------------------------------------

    /**
     * @brief Hilo consumidor de paquetes TTS
     */
    void t_data_consumer();


/************ Variables ********************************************************/

// Aliases

    /**
     * @brief Estructura de información de elementos en reproducción
     * @details Contiene además los datos de TTSPacket recibidos 
     */
    struct TTSMgrInfo : TTSPacket {
        std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida de la asignación voz <-> entidad
        std::string             model_name_assigned;    ///< Nombre del modelo asociado a la entidad
    };
    using TTSInfos      = std::unordered_map<std::string, std::vector<TTSMgrInfo>>;

// Pointer to implementation (PIMPL) para añadir iComm (clase administrada CLI.NET)
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    
// Inicialización y ejecución
    std::atomic<bool>           running_;               ///< flag de aplicación corriendo (para hilos)
    bool                        initialized_;           ///< Bandera para indicar inicialización exitosa
    std::thread                 initTTS_thread_;        ///< Hilo inicializador de TTSCore

// Cola de datos
    std::thread                 dataConsumer_thread_;   ///< Hilo consumidor de datos TTS (queue)
    std::queue<TTSPacket>       queue_;                 ///< Cola de comandos
    std::mutex                  queue_mtx_;             ///< Mutex de cola de comandos
    std::condition_variable     queue_cv_;              ///< Conditional variable para mutex de cola

// Módulos
    TTSCore                     ttsCore_;               ///< Clase núcleo de tts

// Reproductores TTS (usan playback de soundmgr)
    TTSInfos                    PlayersInfo_;           ///< Lista de información de cada PlaybackTTS
    std::mutex                  playersInfo_mtx_;       ///< Mutex para la lista de información

// Observadores
    std::vector<ITTSObserver*>  observers_;             ///< Lista de observadores para transferir datos

};
