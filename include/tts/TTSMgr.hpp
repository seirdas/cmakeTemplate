#pragma once

#include <memory>   // unique_ptr
#include <string>
#include <queue>


// intentar quitar esto si es posible:
#include "tts/TTSDataTypes.hpp"
#include "tts/TTSCore.hpp"


// Declaración implícita
class TTSCore;
class SoundMgr;
class TTSPlayer;


class TTSMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSMgr(SoundMgr* snd = nullptr);
    
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

    
// TTSCore ------------------------------------------------------------------------------

    /**
     * @brief Genera un archivo '.wav' a partir de un texto con una voz determinada
     * @param modelName Nombre del modelo
     * @param text Texto del que generar el audio
     * @param wavname Nombre del archivo resultante
     * @return @c true Si se ha generado correctamente, @c false en caso contrario
     */
    bool generateWav(std::string const& modelName, std::string const& text, std::string wavname);

    /**
     * @brief Obtiene una lista con los nombres de los modelos disponibles
     * @note Los nombres SÍ incluyen el tipo de modelo (ej. "vits-piper-*")
     */
    std::vector<std::string> getAvailableModels();

    /**
     * @brief Obtiene una lista con los nombres de los modelos cargados
     * @note Los nombres NO incluyen el tipo de modelo (ej. "vits-piper-*")
     * @note Con lazy_load activo, los modelos disponibles se consideran como cargados
     */
    std::vector<std::string> getLoadedModels() const;

    /**
     * @brief Obtiene el número de modelos disponibles
     */
    short numAvailableModels() const;

    /**
     * @brief Obtiene el número de modelos cargados
     * @returns Número de modelos cargados disponibles
     *  Con lazy_load activo, NO CUENTA los modelos disponibles no cargados
     */
    short numLoadedModels() const;


// Gestión de reproductores TTS ---------------------------------------------------------

    /**
     * @brief Añade un nuevo reproductor TTS
     * @param TTSPlayerName Nombre del reproductor TTS
     * @param playbackName Nombre del dispositivo playback por el que se reproducen los audios
     *  El dispositivo playback debe estar gestionado por el gestor de audio (SoundMgr)
     * @return @c true si se ha creado el reproductor correctamente, @c false en caso contrario
     */
    bool add_tts_player(std::string const& TTSPlayerName, std::string const& playbackName = "");

    /**
     * @brief Borra un reproductor TTS
     * @param name Nombre del reproductor TTS a borrar
     * @return @c true si se ha eliminado el reproductor correctamente, @c false en caso contrario
     */
    bool remove_tts_player(std::string const& name);


// TTSPlayer ----------------------------------------------------------------------------

    bool play(
        std::string const& text, 
        std::string const& entityName,
        std::string const& modelName = "", 
        std::string const& playbackName = "" );


// Hilos --------------------------------------------------------------------------------

    /**
     * @brief Hilo consumidor de paquetes TTS
     */
    void TWorker();
    

private:

/************ Variables ********************************************************/

// Aliases
    using TTSPlayers    = std::unordered_map<std::string, std::unique_ptr<TTSPlayer>>;

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
    std::atomic<bool>           running_;       ///< flag de aplicación corriendo (para hilos)
    bool                        initialized_;   ///< Bandera para indicar inicialización exitosa
    std::thread                 hilo_ttscore_;  ///< Hilo inicializador de TTSCore

// Cola de datos
    std::thread                 hilo_consumer_; ///< Hilo consumidor de datos TTS (queue)
    std::queue<TTSPacket>       queue_;         ///< Cola de comandos
    std::mutex                  queue_mtx_;     ///< Mutex de cola de comandos
    std::condition_variable     queue_cv_;      ///< Conditional variable para mutex de cola

// Módulos
    SoundMgr*                   snd_;           ///< Puntero a clase de gestión de audio para reproducción
    TTSCore                     ttsCore_;       ///< Clase núcleo de tts

// Reproductores TTS (usan playback de soundmgr)
    TTSPlayers                  ttsPlayers_;        ///< Lista de reproductores TTS
    TTSInfos                    PlayersInfo_;       ///< Lista de información de cada TTSPlayer
    std::mutex                  playersInfo_mtx_;   ///< Mutex para la lista de información

};
