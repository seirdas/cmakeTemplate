#pragma once

#include <string>
#include <functional>
#include "system/SystemMgr.hpp"

class TTSCore;

class TTSPlayer {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer() :
        fn_textToAudio(nullptr),
        fn_audioToPlayback(nullptr),
        active_tasks_(0)
    {

    }
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayer() {

    }


// Ejecución ----------------------------------------------------------------------------

    bool init(std::string const& pbName) {
        playbackName_ = pbName;
        return true;
    }

    bool play(std::string const& modelName, std::string const& text) {

        // Comprobaciones previas
        if (modelName.empty()) {
            SYS_WARN("TTSPlayer","Cannot play: ModelName empty.");
            return false;
        }
        if (text.empty()) {
            SYS_WARN("TTSPlayer","Cannot play: Text empty.");
            return false;
        }
        if (!fn_textToAudio || !fn_audioToPlayback) {
            SYS_WARN("TTSPlayer","Cannot generate audio: Callback functions don't exist.");
            return false;
        }

        // Lanzamos en un hilo para no bloquear al caller
        std::thread([this, modelName, text]() {
            // Incrementar contador de tareas activas de la clase
            active_tasks_++;
            // Generar audio usando función inyectada de tts
            std::vector<float> audio = fn_textToAudio(modelName, text);
            if (audio.empty()) {
                SYS_WARN("TTSPlayer","Empty audio generated from " + modelName);
                return false;
            }
            // Reproducir audio por el playback
            bool result = fn_audioToPlayback(audio, playbackName_);
            // Decrementar contador de tareas activas de la clase
            active_tasks_--;

            if (!result) {
                SYS_WARN("TTSPlayer","Cannot reproduce audio through playback '" + playbackName_ + "'");
                return false;
            }

            // Notificar que ha terminado de reproducir
            /* #TODO */
        }).detach();

        return true;
    }

// Datos --------------------------------------------------------------------------------

    void setPlaybackDev(std::string const& pbName) {
        playbackName_ = pbName;
    }

    bool isBusy() {
        return (active_tasks_ > 0);
    }

// Inyección de funciones ---------------------------------------------------------------

    /**
     * @brief Función inyectada que utiliza el núcleo de tts para pasar un texto a audio
     * @param modelName Nombre del modelo tts a usar
     * @param text Texto para convertir a audio
     * @returns std::vector<float> Vector de muestras de audio en formato float
     */
    using TTSFunction = std::function<std::vector<float>(std::string const& modelName, std::string const& text)>;

    void setTTSCallback(TTSFunction fn) {
        fn_textToAudio = std::move(fn); 
    }

    /**
     * @brief Función inyectada que utiliza el soundMgr para reproducir un audio
     * @param audio Audio a reproducir (obtenido del tts)
     * @param playbackName Nombre del dispositivo playback por el que reproducir el audio
     */
    using PlaybackFunction = std::function<bool(std::vector<float>& audio, std::string const& playbackName)>;


    void setPlaybackCallback(PlaybackFunction fn) {
        fn_audioToPlayback = std::move(fn);
    }


private:

/************ Variables ********************************************************/

    // Funciones inyectadas
    TTSFunction       fn_textToAudio;       ///< Función inyectada para pasar de texto a audio
    PlaybackFunction  fn_audioToPlayback;   ///< Función inyectada para reproducir audio en playback

    // Ejecución 
    std::atomic<short>  active_tasks_;      ///< Indica si hay algo en ejecución

    // Datos
    std::string         playbackName_;      ///< Nombre del playback por el que se reproduce

    // Datos temporales
};
