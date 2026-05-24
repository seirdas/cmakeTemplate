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

    std::vector<std::string> getAvailableInputs() const;

    std::string getDefaultInputDevice() const;

    void listAvailableInputs();

    bool addCaptureDevice(std::string const& name, unsigned short index);

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

    // Estructura PIMPL para no depender de miniaudio en el header
    struct Impl;
    std::unique_ptr<Impl> pimpl_;       // Miembros dependientes de <miniaudio>

    // Módulos de audio
    InputsVector    inputs_;            // Vector con dispositivos inicializados de captura
    PlaybacksVector playbacks_;         // Vector con dispositivos inicializados de playback

    // Estado del módulo
    std::atomic<bool> ctx_initialized_; // Flag para saber si el motor de audio está inicializado.

};
