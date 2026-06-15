#pragma once

#include <atomic>
#include <string>
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
     * @returns True si la inicialización ha sido correcta, false en caso contrario
     */
    bool init();

    /**
    * @brief Para el motor de audio.
    * @returns True si la parada ha sido correcta, false en caso contrario. 
    */
    bool stop();

    /**
     * @brief Actualiza la información de los dispositivos Playback/Capture disponibles
     */
    bool updateDevices();


// Capture Input ------------------------------------------------------------------------

    /**
     * @brief Devuelve una lista con los nombres de todos los micrófonos disponibles
     */
    std::vector<std::string> getAvailableInputs() const;

    /** 
     * @brief Devuelve el nombre del micrófono que tiene Windows marcado como predeterminado.
     */
    std::string getDefaultInputDevice() const;

    /**
     * @brief Añadir como dispositivo de captura
     */ 
    bool addCaptureDevice(std::string const& name, unsigned short index);

    /**
     * @brief Eliminar el dispositivo de captura
     */ 
    bool removeInputDevice(unsigned short index); 

    /**
     * @brief Empieza a grabar
     */
    bool startRec_snd(unsigned short index);

    /**
     * @brief Para de grabar
     */
    bool stopRec_snd(unsigned short index);

    /**
     * @brief Obtiene el nivel de RMS del audio
     */
    float getInputRmsLevel(unsigned short index);

    /**
     * @brief Obtiene el tamaño del buffer de captura
     * @param index indice de dispositivo de captura
     * @return el tamaño del buffer de captura
     */
    size_t getInputBufferSize(unsigned int index);
    
    /**
     * @brief Obtiene el tamaño del buffer de grabación
     * @param index indice de dispositivo de captura
     * @return el tamaño del buffer de grabación
     */
    size_t getInputRecBufferSize(unsigned int index);

    /**
     * @brief Comprobar si el dispositivo sigue activo y funcionando
     */
    bool isInputDeviceValid(unsigned short index) const;


// Playbacks ----------------------------------------------------------------------------

    std::vector<std::string> getAvailablePlaybacks() const;

    std::string getDefaultPlaybackDevice() const;

    void listAvailablePlaybacks();

    bool addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder);

    bool removePlaybackDevice(unsigned short index);

    bool playbackTest();


private:

/************ Variables ****************************************************************/

    using PlaybacksVector = std::vector<std::unique_ptr<AudioPlaybackModule>>;
    using InputsVector = std::vector<std::unique_ptr<AudioInputModule>>;

    // Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de la librería externa

    // Módulos de audio
    InputsVector    inputs_;            ///< Vector con dispositivos inicializados de captura
    PlaybacksVector playbacks_;         ///< Vector con dispositivos inicializados de playback

    // Estado del módulo
    std::atomic<bool> ctx_initialized_; ///< Flag para saber si el motor de audio está inicializado.
    int MAX_REINIT_ATTEMPTS = 3;        ///< Número de reintentos para reinicializar dispositivo de entrada
};
