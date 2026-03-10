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

    /**
     * @brief Actualiza la información de los dispositivos Playback/Capture
     */
    bool updateDevices();


// Capture Input ------------------------------------------------------------------------

    std::vector<std::string> getAvailableInputs() const;

    void listAvailableInputs() const;

    bool addCaptureDevice(std::string const& name, unsigned short index);

// Playbacks ----------------------------------------------------------------------------

    std::vector<std::string> getAvailablePlaybacks() const;

    void listAvailablePlaybacks() const;

    bool addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder);

    bool removePlaybackDevice(unsigned short index);

    bool playbackTest();

private:

    /************ Variables ********************************************************/

    // Motor de audio
    ma_context snd_context_;                        // Contexto de audio
    std::atomic<bool> ctx_initialized_;             // Flag para saber si el motor de audio está inicializado.

    ma_device_info* pPlaybackDevInfos_   = nullptr;     // Información de dispositivos Playback
    ma_uint32       PlaybackDevCount_    = 0;           // Número de playbacks disponibles
    ma_device_info* pCaptureDeviceInfos_ = nullptr;     // Información de dispositivos de captura (Input)
    ma_uint32       captureDeviceCount_  = 0;           // Número de inputs disponibles

    // Modulos de audio 
    std::vector<std::unique_ptr<AudioInputModule>>      inputs_;        // Módulos de entrada
    std::vector<std::unique_ptr<AudioPlaybackModule>>   playbacks_;     // Módulos Playback
};
