#pragma once

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
    std::vector<std::string> getAvailableInputs() const;

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
     * @brief Devuelve el nombre del micrófono que tiene Windows marcado como predeterminado.
     */
    std::string getDefaultInputDevice() const;

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
     */ 
    bool removeInputDevice(std::string const& name); 


// Ejecución y datos en dispositivos de captura -----------------------------------------

    /**
     * @brief Empieza a grabar
     */
    bool startRec(std::string const& name);

    /**
     * @brief Para de grabar
     */
    bool stopRec(std::string const& name);

    /**
     * @brief Obtiene el nivel de RMS del audio
     */
    float getInputRmsLevel(std::string const& name);

    /**
     * @brief Obtiene el nivel del pico del audio
     */
    float getInputPeakLevel(std::string const& name);

    /**
     * @brief Comprobar si el dispositivo sigue activo y funcionando
     */
    bool isInputDeviceValid(std::string const& name) const;

     /**
      * @brief Tamaño del buffer del dispositivo de grabación
      */
    size_t getInputRecBufferSize(std::string const& name);
    
    /**
      * @brief Tamaño del buffer del dispositivo de captura
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

    bool addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder);

    bool removePlaybackDevice(std::string const& name);

    /**
    * @brief Registra los playbacks Dante de cada grupo de tonos (dynamic/morse/radio) según el JSON.
     * @param playbackConfig Puntero al sub-nodo "Playback" del JSON, con la configuración de cada grupo.
    */
    int initTonePools(void* playbackConfig);



// Ejecución y datos de playbacks -------------------------------------------------------

    bool playbackTest();


private:

/************ Variables ****************************************************************/

// Aliases
    using CapturesList  = std::unordered_map<std::string, std::unique_ptr<AudioInputModule>>;
    using PlaybacksList = std::unordered_map<std::string, std::unique_ptr<AudioPlaybackModule>>;

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;         ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                    initialized_;           ///< Bandera para indicar inicialización exitosa
    unsigned short          MAX_REINIT_ATTEMPTS;    ///< Número de reintentos para reinicializar dispositivo de entrada

// Módulos de audio
    CapturesList            captures_;              ///< Vector con dispositivos inicializados de captura
    PlaybacksList           playbacks_;             ///< Vector con dispositivos inicializados de playback
    
    std::unordered_map<std::string, std::vector<std::string>> tonePools_;  ///< Pools de tonos: nombre del grupo -> lista de playbacks Dante asignados


// Parámetros de los módulos capture/playbacks
    bool                    smoothedValues_;        ///< Suaviza los valores obtenidos en los módulos (peak, rms...)
    float                   attackCoeff_;           ///< Valor de ataque (+grande = subida lenta)
    float                   releaseCoeff_;          ///< Valor de release (+grande = bajada lenta)

};
