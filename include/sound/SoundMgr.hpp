#pragma once

#include <miniaudio.h>
#include <atomic>
#include <string>
#include <vector>
#include <memory>                       // unique_ptr

// Evita los includes:

class AudioInputModule;
class AudioPlaybackModule;

/**
  * @class SoundMgr
  * @brief Gestor de Audio Inputs y Playbacks de audio.
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
    * @returns True Si la parada ha sido correcta, false en caso contrario. 
    */
    bool stop();

    bool listInputDevices();

    std::vector<std::string> getAvailableInputDevices();

    bool listOutputDevices();

// Capture Input ------------------------------------------------------------------------

    bool addCaptureDevice(std::string const& name, unsigned short index);

// Playbacks ----------------------------------------------------------------------------

    bool addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder);

    bool removePlaybackDevice(unsigned short index);

    unsigned short getPlaybackDevice(std::string const& deviceName);

    bool playbackTest();

private:

    /************ Variables ********************************************************/

    // Motor de audio
    ma_context snd_context_;                        // Contexto de audio
    std::atomic<bool> ctx_initialized_;             // Flag para saber si el motor de audio está inicializado.

    // Modulos de audio 
    std::vector<std::unique_ptr<AudioInputModule>>      inputs_;        // Módulos de entrada
    std::vector<std::unique_ptr<AudioPlaybackModule>>   playbacks_;     // Módulos Playback
};
