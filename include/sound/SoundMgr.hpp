#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>                       // unique_ptr


// Evita los includes de los módulos:
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
    * @returns True si la parada ha sido correcta, false en caso contrario.
    */
    bool stop();

    /**
     * @brief Actualiza la información de los dispositivos Playback/Capture disponibles
     */
    bool updateDevices();


// Dispositivos de captura --------------------------------------------------------------

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
     * @brief Añadir un nuevo dispositivo de captura a partir de una config (json)
     * @param config Puntero a parámetros de dispositivo (json)
     * @param name Nombre asignado a esta captura (cualquiera)
     * @param deviceName Nombre del dispositivo de captura
     * @return @c true Si se ha creado correctamente, @c false en caso contrario
     */
    bool addCaptureDevice(void* config, std::string const& captureName = "", std::string const& deviceName = "");

    /**
     * @brief Eliminar el dispositivo de captura
     * @param name Nombre del dispositivo de captura a eliminar
     */
    bool removeCaptureDevice(std::string const& name);


// Ejecución y datos en dispositivos de captura -----------------------------------------

    /**
     * @brief Empieza a grabar en un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
     */
    bool startRec(std::string const& name);

    /**
     * @brief Para de grabar en un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
     */
    bool stopRec(std::string const& name);

    /**
     * @brief Obtiene el nivel de RMS del audio
     *  en un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
     */
    float getInputRmsLevel(std::string const& name);

    /**
     * @brief Obtiene el nivel del pico del audio
     *  en un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
     */
    float getInputPeakLevel(std::string const& name);

    /**
     * @brief Comprobar si el dispositivo sigue activo y funcionando
     *  en un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
     */
    bool isInputDeviceValid(std::string const& name) const;

     /**
      * @brief Tamaño del buffer del dispositivo de grabación
     *  de un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
      */
    size_t getInputRecBufferSize(std::string const& name);

    /**
      * @brief Tamaño del buffer del dispositivo de captura
     *  de un dispositivo de captura determinado
     * @param name Nombre del dispositivo de captura seleccionado
      */
    size_t getInputBufferSize(std::string const& name);


// Gestión de dispositivos playbacks ----------------------------------------------------

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

    std::string getDefaultPlaybackDevice() const;

    void listAvailablePlaybacks() const;

    std::string addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder);

    bool removePlaybackDevice(std::string const& name);

    /**
    * @brief Registra los playbacks Dante de cada grupo de tonos (dynamic/morse/radio) según el JSON.
     * @param playbackConfig Puntero al sub-nodo "Playback" del JSON, con la configuración de cada grupo.
    */
    int initTonePools(void* playbackConfig);



// Ejecución y datos de playbacks -------------------------------------------------------

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
    std::vector<float> generateMorse(std::string const& tipo, std::string const& texto) const;


/************ Variables ****************************************************************/

// Aliases
    using CapturesList  = std::unordered_map<std::string, std::unique_ptr<AudioInputModule>>;
    using PlaybacksList = std::unordered_map<std::string, std::unique_ptr<AudioPlaybackModule>>;
    using TonePoolsList = std::unordered_map<std::string, std::vector<std::string>>;

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;         ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                    initialized_;           ///< Bandera para indicar inicialización exitosa
    unsigned short          MAX_REINIT_ATTEMPTS;    ///< Número de reintentos para reinicializar dispositivo de entrada

// Listas de dispositivos de audio
    std::vector<std::string>    available_inputs_;      ///< Lista de dispositivos de entrada disponibles
    std::vector<std::string>    managed_inputs_;        ///< Lista de dispositivos de entrada gestionados
    std::vector<std::string>    available_playbacks_;   ///< Lista de dispositivos playback disponibles
    std::vector<std::string>    managed_playbacks_;     ///< Lista de dispositivos playback gestionados

    mutable std::mutex          available_inputs_mtx_;      ///< Mutex para lista de dispositivos de entrada disponibles
    mutable std::mutex          managed_inputs_mtx_;        ///< Mutex para lista de dispositivos de entrada gestionados
    mutable std::mutex          available_playbacks_mtx_;   ///< Mutex para lista de dispositivos playback disponibles
    mutable std::mutex          managed_playbacks_mtx_;     ///< Mutex para lista de dispositivos playback gestionados

// Módulos de audio
    CapturesList            captures_;              ///< Vector con dispositivos inicializados de captura
    PlaybacksList           playbacks_;             ///< Vector con dispositivos inicializados de playback
    TonePoolsList           tonePools_;             ///< Pools de tonos: nombre del grupo -> lista de playbacks asignados

// Parámetros de los módulos capture/playbacks
    bool                    smoothedValues_;        ///< Suaviza los valores obtenidos en los módulos (peak, rms...)
    float                   attackCoeff_;           ///< Valor de ataque (+grande = subida lenta)
    float                   releaseCoeff_;          ///< Valor de release (+grande = bajada lenta)

// Playbacks: Parámetros Morse
    unsigned int                                  morseUnitMs_;           ///< Duración del punto, compartida por todo el grupo MORSE
    unsigned int                                  morseSampleRate_;       ///< Sample rate (Hz) del audio generado para morse
    std::unordered_map<std::string, float>        morseFrequencies_;      ///< Por radioayuda: frecuencia del tono (Hz)
    std::unordered_map<std::string, unsigned int> espacioEntreMorse_;     ///< Por radioayuda: separación entre palabras (ms)

};
