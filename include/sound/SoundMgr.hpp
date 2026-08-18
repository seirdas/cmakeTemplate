#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>           // unique_ptr
#include <thread>



// Forward declaration (evita includes en hpp)
class AudioCaptureModule; 
class AudioPlaybackModule;
class PlayerAudio;
class PlayerMorse;
class PlayerTTS;
class TTSCore;


/**
  * @class SoundMgr
  * @brief Gestor de módulos de captura y reproducción de audio.
  */
class SoundMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor.
     */
    SoundMgr();

    /**
     * @brief Destructor.
     */
    ~SoundMgr();
    
    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    SoundMgr(SoundMgr const&) = delete;
    SoundMgr& operator=(SoundMgr const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    SoundMgr(SoundMgr&&) = delete;
    SoundMgr& operator=(SoundMgr&&) = delete;


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicialización del motor de audio.
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa.
     * @return @c true Si ha iniciado bien, @c false en caso contrario.
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
    * @brief Para el motor de audio.
    * @returns @c true si la parada ha sido correcta, @c false en caso contrario. 
    */
    bool close();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Actualiza la información de los dispositivos Playback/Capture disponibles.
     */
    bool updateDevices();

    /**
     * @brief Realiza una prueba de reproducción a través del playback por defecto, 
     *  utilizando la arquitectura de la clase (lista de playbacks, inicialización, etc.)
     * @return @c true Si la prueba se ha realiza con éxito, @c false en caso contrario.
     */
    bool playbackTest();


// Módulos de Captura -------------------------------------------------------------------

    /**
     * @brief Añadir un nuevo dispositivo de captura.
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto.
     * @note Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado.
     * @param config Puntero a configuración (json).
     * @param moduleName Nombre asignado a este módulo (cualquiera).
     * @param deviceName Nombre del dispositivo de captura.
     * @return @c true Si se ha creado correctamente, @c false en caso contrario.
     */
    bool addCaptureDevice(
        void*               config, 
        std::string const&  moduleName, 
        std::string const&  deviceName  = ""
    );

    /**
     * @brief Eliminar el dispositivo de captura.
     * @param moduleName Nombre del módulo seleccionado.
     */
    bool removeCaptureDevice(std::string const& moduleName);

    /**
     * @brief Obtiene el módulo de captura asociado a un nombre
     *  para utilizar las funciones del módulo de captura
     * @param moduleName Nombre del módulo seleccionado
     * @return Puntero al módulo de captura o @c nullptr si no existe con ese nombre
     */
    AudioCaptureModule* getCapture(std::string moduleName) const;

    /**
     * @brief Devuelve una lista con los nombres de todos
     *  los módulos de captura agregados.
     */
    std::vector<std::string> getCaptureModuleNames() const;


// Módulos PlayerAudio ------------------------------------------------------------------

    /**
     * @brief Añadir un nuevo módulo de reproducción playback de archivos de audio
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto
     * @details Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado
     * @param config Puntero a configuración (json)
     * @param moduleName Nombre asignado a este módulo (cualquiera)
     * @param deviceName Nombre del dispositivo playback
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addPlayerAudio(
        void*               config,
        std::string const&  moduleName, 
        std::string const&  deviceName
    );

    /**
     * @brief Eliminar el módulo de reproducción playback de archivos de audio
     * @param moduleName Nombre del módulo seleccionado
     */ 
    bool removePlayerAudio(std::string const& moduleName);

    /**
     * @brief Obtiene el módulo de reproducción playback de archivos de audio 
     *  asociado a un nombre para utilizar las funciones del módulo playback
     * @param moduleName Nombre del módulo seleccionado
     * @return Puntero al módulo de reproducción o @c nullptr si no existe con ese nombre
     */
    PlayerAudio* getPlayerAudio(std::string moduleName) const;


// Módulos PlayerMorse ------------------------------------------------------------------

    /**
     * @brief Añadir un nuevo módulo de reproducción playback de texto en morse
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto
     * @details Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado
     * @param config Puntero a configuración (json)
     * @param moduleName Nombre asignado a este módulo (cualquiera)
     * @param deviceName Nombre del dispositivo playback
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addPlayerMorse(
        void*               config,
        std::string const&  moduleName, 
        std::string const&  deviceName
    );

    /**
     * @brief Eliminar el módulo de reproducción playback de texto en morse
     * @param moduleName Nombre del módulo a eliminar
     */ 
    bool removePlayerMorse(std::string const& moduleName);

    /**
     * @brief Obtiene el módulo de reproducción playback de texto en morse 
     *  asociado a un nombre para utilizar las funciones del módulo playback
     * @param moduleName Nombre del módulo seleccionado
     * @return Puntero al módulo de reproducción o @c nullptr si no existe con ese nombre
     */
    PlayerMorse* getPlayerMorse(std::string moduleName) const;


// Módulos PlayerTTS --------------------------------------------------------------------

    /**
     * @brief Añadir un nuevo módulo de reproducción playback de texto a voz (TTS)
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto
     * @details Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado
     * @param config Puntero a configuración (json)
     * @param moduleName Nombre asignado a este módulo (cualquiera)
     * @param deviceName Nombre del dispositivo playback
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addPlayerTTS(
        void*               config,
        std::string const&  moduleName, 
        std::string const&  deviceName
    );

    /**
     * @brief Eliminar el módulo de reproducción playback de texto a voz (TTS)
     * @param moduleName Nombre del módulo seleccionado
     */ 
    bool removePlayerTTS(std::string const& moduleName);

    /**
     * @brief Obtiene el módulo de reproducción playback de texto a voz (TTS) 
     *  asociado a un nombre para utilizar las funciones del módulo playback
     * @param moduleName Nombre del módulo seleccionado
     * @return Puntero al módulo de reproducción o @c nullptr si no existe con ese nombre
     */
    PlayerTTS* getPlayerTTS(std::string moduleName) const;

    /**
     * @brief Obtiene el gestor de TTS
     */
    TTSCore* getTTSCore() const;


// Datos de dispositivos ----------------------------------------------------------------

    /**
     * @brief Devuelve una lista con los nombres de todos
     *  los dispositivos de entrada disponibles
     */
    std::vector<std::string> getAvailableCaptures() const;

    /**
     * @brief Devuelve el nombre del dispositivo de captura
     *  marcado como predeterminado.
     */
    std::string getDefaultCaptureDevice() const;

    /**
     * @brief Muestra en el log del sistema la lista
     *  de dispositivos de captura disponibles
     */
    void listAvailableCaptures() const;

    /**
     * @brief Obtiene el dispositivo de reproducción playback predeterminado
     * @return Nombre del dispositivo de reproducción (predeterminado)
     */
    std::string getDefaultPlaybackDevice() const;

    /**
     * @brief Devuelve una lista con los nombres de todos
     *  los dispositivos de reproducción disponibles
     */
    std::vector<std::string> getAvailablePlaybacks() const;

    /**
     * @brief Muestra en el log del sistema la lista
     *  de dispositivos de captura disponibles
     */
    void listAvailablePlaybacks() const;


private:

// Funciones internas auxiliares --------------------------------------------------------

    /**
     * @brief Busca el ma_device_info a partir de un nombre
     * @note Localiza también por nombres incompletos ( 'tavo' -> 'Altavoces' ),
     *  sustituyendo el nombre por parámetro por el nombre completo real
     * @param myDeviceName Nombre del dispositivo
     *  MODIFICABLE al nombre real completo si aplica
     * @param isCapture Indica si es de la lista de dispositivos de captura. 
     *  En su defecto, se utilizará la lista de dispositivos de reproducción playback
     * @return (ma_device_info*) Información del dispositivo (si se encuentra)
     */
    const void* get_device_info(std::string& myDeviceName, bool isCapture) const;

    /**
     * @brief "Shortcut" para obtener ma_device_info de 
     *  un dispositivo de reproducción playback a partir del nombre
     * @param myDeviceName Nombre del dispositivo
     *  MODIFICABLE al nombre real completo si aplica
     * @return (ma_device_info*) Información del dispositivo (si se encuentra)
     */
    const void* get_playback_device_info(std::string& myDeviceName) const;

    /**
     * @brief "Shortcut" para obtener ma_device_info de 
     *  un dispositivo de captura a partir del nombre
     * @param myDeviceName Nombre del dispositivo
     *  MODIFICABLE al nombre real completo si aplica
     * @return (ma_device_info*) Información del dispositivo (si se encuentra)
     */
    const void* get_capture_device_info(std::string& myDeviceName) const;

    /**
     * @brief Helper común para eliminar un módulo de una lista
     * @tparam MapT 
     * @param name Nombre del módulo
     * @param map Mapa del que eliminar el módulo
     * @return @c true Si el módulo se ha eliminado, @c false en caso contrario
     */
    template <typename MapT>
    bool remove_module(std::string const& name, MapT& map);
    

// Listas de dispositivos de audio ------------------------------------------------------

    /**
     * @brief Actualiza la lista de dispositivos de entrada disponibles
     */
    void update_available_inputs();

    /**
     * @brief Actualiza la lista de dispositivos playback disponibles
     */
    void update_available_playbacks();


/************ Variables ****************************************************************/

// Aliases
    using CapturesList      = std::unordered_map<std::string, std::unique_ptr<AudioCaptureModule>>;
    using PlayersAudioList  = std::unordered_map<std::string, std::unique_ptr<PlayerAudio>>;
    using PlayersMorseList  = std::unordered_map<std::string, std::unique_ptr<PlayerMorse>>;
    using PlayerTTSList     = std::unordered_map<std::string, std::unique_ptr<PlayerTTS>>;
    using TonePoolsList     = std::unordered_map<std::string, std::vector<std::string>>;

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;
    std::unique_ptr<Impl>       pimpl_;                     ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                        initialized_;               ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>           running_;                   ///< flag de aplicación corriendo (para hilos)
    unsigned short              MAX_REINIT_ATTEMPTS;        ///< Número de reintentos para reinicializar dispositivo de entrada

// Listas de dispositivos de audio
    std::vector<std::string>    available_inputs_;          ///< Lista de dispositivos de entrada disponibles
    std::vector<std::string>    available_playbacks_;       ///< Lista de dispositivos playback disponibles
    mutable std::mutex          available_inputs_mtx_;      ///< Mutex para lista de dispositivos de entrada disponibles
    mutable std::mutex          available_playbacks_mtx_;   ///< Mutex para lista de dispositivos playback disponibles

// Módulos de audio
    CapturesList                captures_;                  ///< Vector con dispositivos inicializados de captura
    PlayersAudioList            playersAudios_;             ///< Lista con módulos reproductores de archivos
    PlayersMorseList            playersMorse_;              ///< Lista con módulos reproductores de morse
    PlayerTTSList               playersTTS_;                ///< Lista con módulos reproductores de TTS
    
// Módulo TTS
    std::unique_ptr<TTSCore>    tts_;                       ///< Clase núcleo de tts
    std::thread                 initTTS_thread_;            ///< Hilo inicializador de TTSCore

// Parámetros de los módulos capture/playbacks
    bool                        enabledSmoothedValues_;     ///< Suaviza los valores obtenidos en los módulos (peak, rms...)
    float                       attackCoeff_;               ///< Valor de ataque (+grande = subida lenta)
    float                       releaseCoeff_;              ///< Valor de release (+grande = bajada lenta)

};
