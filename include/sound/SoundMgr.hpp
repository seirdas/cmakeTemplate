#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>                       // unique_ptr


// Forward declaration
class AudioInputModule;         // Evita el include de AudioInputModule
class AudioPlaybackModule;      // Evita el include de AudioPlaybackModule


/**
  * @class SoundMgr
  * @brief Gestor de Inputs y Playbacks de audio.
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

    /**
     * @brief Inicialización del motor de audio.
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
    * @brief Para el motor de audio.
    * @returns @c true si la parada ha sido correcta, @c false en caso contrario. 
    */
    bool close();

    /**
     * @brief Actualiza la información de los dispositivos Playback/Capture disponibles
     */
    bool updateDevices();


// Dispositivos de Captura --------------------------------------------------------------

    /**
     * @brief Obtiene el módulo de captura asociado a un nombre
     *  para utilizar las funciones del módulo de captura
     * @param playbackName Nombre del dispositivo de captura seleccionado
     * @return Puntero al módulo de captura o @c nullptr si no existe con ese nombre
     */
    AudioInputModule* getCapture(std::string captureName) const;

    /**
     * @brief Devuelve una lista con los nombres de todos
     *  los dispositivos de entrada disponibles
     */
    std::vector<std::string> getAvailableCaptures() const;

    /**
     * @brief Devuelve una lista con los nombres de todas
     *  las capturas agregadas
     */
    std::vector<std::string> getManagedCaptures() const;

    /**
     * @brief Devuelve si un dispositivo de captura pasado por parámetro
     *  está siendo gestionado
     * @param captureName Nombre del dispositivo
     * @return @c true Si está siendo gestionado, @c false en caso contrario
     */
    bool isOnManagedCaptures(std::string const& captureName) const;

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
     * @brief Añadir un nuevo dispositivo de captura
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto
     * @note Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado
     * @param config Puntero a configuración (json)
     * @param name Nombre asignado a esta captura (cualquiera)
     * @param deviceName Nombre del dispositivo de captura
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addCaptureDevice(
        void*               config, 
        std::string const&  captureName = "", 
        std::string const&  deviceName  = ""
    );

    /**
     * @brief Eliminar el dispositivo de captura
     * @param name Nombre del dispositivo de captura a eliminar
     */
    bool removeCaptureDevice(std::string const& captureName); 


// Dispositivos Playback ----------------------------------------------------------------

    /**
     * @brief Obtiene el módulo de reproducción playback asociado a un nombre
     *  para utilizar las funciones del módulo playback
     * @param playbackName Nombre del dispositivo de reproducción playback seleccionado
     * @return Puntero al módulo de reproducción o @c nullptr si no existe con ese nombre
     */
    AudioPlaybackModule* getPlayback(std::string captureName) const;

    /**
     * @brief Devuelve una lista con los nombres de todos
     *  los dispositivos de reproducción disponibles
     */
    std::vector<std::string> getAvailablePlaybacks() const;

    /**
     * @brief Devuelve una lista con los nombres de todas
     *  los playbacks agregados
     */
    std::vector<std::string> getManagedPlaybacks() const;

    /**
     * @brief Devuelve si un dispositivo de playback pasado por parámetro
     *  está siendo gestionado
     * @param captureName Nombre del dispositivo
     * @return @c true Si está siendo gestionado, @c false en caso contrario
     */
    bool isOnManagedPlaybacks(std::string const& captureName) const;

    /**
     * @brief Obtiene el dispositivo de reproducción playback predeterminado
     * @return Nombre del dispositivo de reproducción (predeterminado)
     */
    std::string getDefaultPlaybackDevice() const;

    /**
     * @brief Muestra en el log del sistema la lista
     *  de dispositivos de reproducción playback disponibles
     */
    void listAvailablePlaybacks() const;

    /**
     * @brief Añadir un nuevo dispositivo de reproducción playback
     * @details Usa los datos de la config (json) si se proporciona, 
     *  si no, se inicializará con los valores por defecto
     * @details Si hay algún nombre en captureName y deviceName, sobreescribirán
     *  los valores de la configuración si se hubiera proporcionado
     * @param config Puntero a configuración (json)
     * @param name Nombre asignado a esta captura (cualquiera)
     * @param deviceName Nombre del dispositivo de captura
     * @param AudioFilesFolder Nombre de la carpeta de la que se leerán los audios por defecto
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addPlaybackDevice(
        void*               config,
        std::string const&  playbackName, 
        std::string const&  deviceName,
        std::string const&  AudioFilesFolder = ""
    );

    /**
     * @brief Eliminar el dispositivo de reproducción playback
     * @param playbackName Nombre del dispositivo de captura a eliminar
     */ 
    bool removePlaybackDevice(std::string const& playbackName);

    /**
    * @brief Registra los playbacks Dante de cada grupo de tonos (dynamic/morse/radio) según el JSON.
     * @param playbackConfig Puntero al sub-nodo "Playback" del JSON, con la configuración de cada grupo.
    */
    int initTonePools(void* playbackConfig);


// Ejecución y datos de playbacks -------------------------------------------------------

    /**
     * @brief Realiza una prueba de reproducción a través del playback por defecto, 
     *  utilizando la arquitectura de la clase (lista de playbacks, inicialización, etc.)
     * @return @c true Si la prueba se ha realiza con éxito, @c false en caso contrario
     */
    bool playbackTest();

    /**
    * @brief Genera y reproduce un mensaje morse por el primer playback libre del pool de MORSE.
    * @param toneLabel Etiqueta del tono (se usará como nombre del sonido, ej. "A").
    * @param tipo Radioayuda a la que pertenece el mensaje (ADF1, VOR1...).
    * @param texto Texto a codificar en morse.
    * @param volume Volumen del sonido (0 a 100).
    * @param loop Modo de repetición.
    * @return true si se ha encontrado un playback libre y se ha empezado a reproducir, false en caso contrario.
    */
    bool playMorse(std::string const& toneLabel, std::string const& tipo, std::string const& texto, unsigned short volume = 100, bool loop = false);

    /**
    * @brief Para un tono activo por su etiqueta (la misma que se usó al lanzarlo).
    * @param toneLabel Etiqueta del tono a parar.
    * @return true si se ha encontrado y parado, false si no había ningún tono activo con esa etiqueta.
    */
    bool stopTone(std::string const& toneLabel);


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


// Listas de dispositivos de audio ------------------------------------------------------

    /**
     * @brief Actualiza la lista de dispositivos de entrada disponibles
     */
    void update_available_inputs();

    /**
     * @brief Actualiza la lista de dispositivos de entrada gestionados
     */
    void update_managed_inputs();

    /**
     * @brief Actualiza la lista de dispositivos playback disponibles
     */
    void update_available_playbacks();

    /**
     * @brief Actualiza la lista de dispositivos playback gestionados
     */
    void update_managed_playbacks();


// Morse --------------------------------------------------------------------------------

    /**
    * @brief Genera el audio (PCM mono, float 32) correspondiente a un texto en morse.
    * @param tipo Radioayuda a la que pertenece el mensaje (ADF1, VOR1...), usada para
    *  obtener su frecuencia de tono y el espaciado entre palabras desde la config.
    * @param texto Texto a codificar (letras/números soportados por MORSE_DICT; los
    *  espacios se interpretan como separación entre palabras).
    * @return Vector de muestras PCM (mono, @c morseSampleRate_ Hz). Vacío si el tipo no existe.
    */
    std::vector<float> generate_morse(std::string const& tipo, std::string const& texto) const;


/************ Variables ****************************************************************/

// Aliases
    using CapturesList  = std::unordered_map<std::string, std::unique_ptr<AudioInputModule>>;
    using PlaybacksList = std::unordered_map<std::string, std::unique_ptr<AudioPlaybackModule>>;
    using TonePoolsList = std::unordered_map<std::string, std::vector<std::string>>;

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;
    std::unique_ptr<Impl>       pimpl_;                     ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                        initialized_;               ///< Bandera para indicar inicialización exitosa
    unsigned short              MAX_REINIT_ATTEMPTS;        ///< Número de reintentos para reinicializar dispositivo de entrada

// Listas de dispositivos de audio
    std::vector<std::string>    available_inputs_;          ///< Lista de dispositivos de entrada disponibles
    std::vector<std::string>    managed_inputs_;            ///< Lista de dispositivos de entrada gestionados
    std::vector<std::string>    available_playbacks_;       ///< Lista de dispositivos playback disponibles
    std::vector<std::string>    managed_playbacks_;         ///< Lista de dispositivos playback gestionados

    mutable std::mutex          available_inputs_mtx_;      ///< Mutex para lista de dispositivos de entrada disponibles
    mutable std::mutex          managed_inputs_mtx_;        ///< Mutex para lista de dispositivos de entrada gestionados
    mutable std::mutex          available_playbacks_mtx_;   ///< Mutex para lista de dispositivos playback disponibles
    mutable std::mutex          managed_playbacks_mtx_;     ///< Mutex para lista de dispositivos playback gestionados

// Módulos de audio
    CapturesList                captures_;                  ///< Vector con dispositivos inicializados de captura
    PlaybacksList               playbacks_;                 ///< Vector con dispositivos inicializados de playback
    TonePoolsList               tonePools_;                 ///< Pools de tonos: nombre del grupo -> lista de playbacks asignados
    
// Parámetros de los módulos capture/playbacks
    bool                        enabledSmoothedValues_;     ///< Suaviza los valores obtenidos en los módulos (peak, rms...)
    float                       attackCoeff_;               ///< Valor de ataque (+grande = subida lenta)
    float                       releaseCoeff_;              ///< Valor de release (+grande = bajada lenta)

// Playbacks: Parámetros Morse
    unsigned int                                  morseUnitMs_;              ///< Duración del punto, compartida por todo el grupo MORSE
    unsigned int                                  morseRayaMs_;              ///< Duración de la raya (ms), compartida por todo el grupo MORSE
    unsigned int                                  morseEspacioEntreSimbolos_;///< Silencio entre símbolos de la misma letra (ms)
    unsigned int                                  morseEspacioEntreLetras_;  ///< Silencio entre letras de la misma palabra (ms)
    unsigned int                                  morseSampleRate_;       ///< Sample rate (Hz) del audio generado para morse
    std::unordered_map<std::string, float>        morseFrequencies_;      ///< Por radioayuda: frecuencia del tono (Hz)
    std::unordered_map<std::string, unsigned int> espacioEntreMorse_;     ///< Por radioayuda: separación entre palabras (ms)

};
