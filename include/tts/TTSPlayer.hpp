#pragma once

#include <string>
#include <functional>
#include "system/SystemMgr.hpp"
#include <thread>
#include <queue>

class TTSCore;

class TTSPlayer {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer(std::string const& playerName) :
        fn_textToAudio(nullptr),
        fn_audioToPlayback(nullptr),
        name_(playerName),
        active_tasks_(0),
        initialized_(false),
        running_(false)
    {

    }
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayer() {
        close();
    }


// Ejecución ----------------------------------------------------------------------------

    bool init(std::string const& pbName) {
        playbackName_ = pbName;

        // Lanza el hilo que consumirá la cola interna
        worker_thread_ = std::thread(&TTSPlayer::TProcesarCola, this);

        initialized_    = true;
        running_        = true;
        return initialized_;    // <- true
    }

    bool isInitialized() const {
        return initialized_;
    }

    bool close() {
        {
            std::lock_guard<std::mutex> lock(cola_textos_mtx_);
            running_ = false;
        }
        cola_textos_cv_.notify_one();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

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

    std::string getPlaybackName() const {
        return playbackName_;
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


// Hilos --------------------------------------------------------------------------------

    void TProcesarCola() {
        // #TODO

        while(running_) {
            // #TODO

            break;
        }
    }


private:

/************ Variables ********************************************************/

// Funciones inyectadas
    TTSFunction             fn_textToAudio;         ///< Función inyectada para pasar de texto a audio
    PlaybackFunction        fn_audioToPlayback;     ///< Función inyectada para reproducir audio en playback

// Inicialización y ejecución
    bool                    initialized_;           ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>       running_;               ///< Flag de módulo corriendo (para hilos)
    std::atomic<short>      active_tasks_;          ///< Número de tareas en ejecución

// Datos
    std::string const       name_;                  ///< Nombre asignado a este TTSPlayer
    std::string             playbackName_;          ///< Nombre del playback por el que se reproduce
    std::string             texto_en_proceso_;      ///< Texto que está procesando (generando->reproduciendo) el módulo

// Cola de textos a procesar

    /**
     * @brief Elemento de la cola de textos
     */
    struct queueElement {
        std::string text;       ///< Texto a reproducir
        std::string modelName;  ///< Nombre del modelo de voz TTS a usar
    };
    std::queue<queueElement>    cola_textos_;       ///< Cola de textos pendientes de reproducir
    std::mutex                  cola_textos_mtx_;   ///< Mutex para cola de textos
    std::condition_variable     cola_textos_cv_;    ///< Condition variable para cola de textos
    std::thread                 worker_thread_;     ///< Hilo dedicado a procesar la cola de textos

};
