#pragma once

#include <memory>   // unique_ptr
#include <string>

/* temp para que funcione en el hpp */
#include "tts/TTSPlayer.hpp"
#include "tts/TTSData.hpp"
#include "tts/TTSCore.hpp"
#include "files/JsonMgr.hpp"

#include "Sound/SoundMgr.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
/************************************/


// Declaración implícita
// class TTSCore;
// class SoundMgr;


class TTSMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSMgr(SoundMgr* snd = nullptr) :
        initialized_(false),
        running_(false),
        snd_(snd)
    {

    }
    
    /**
     * @brief Destructor 
     */
    ~TTSMgr() {
        cerrar();
    }


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config) {

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else
            SYS_WARN("TTSMgr","Cannot load config. Using default values.");
        
        // Inicialización de TTSCore (en hilo para no bloquear)
        SYS_INFO("TTSMgr","Starting TTSCore async load...");
        hilo_ttscore_ = std::thread([this, config]() {
                if(!ttsCore_.init(config))
                    SYS_WARN("TTSMgr","TTSCore FAIL");
                else SYS_INFO("TTSMgr","TTSCore OK");
                JsonMgr::instance().update();
            }
        );

        // Hilo consumidor de paquetes TTS
        hilo_consumer_ = std::thread(&TTSMgr::TWorker, this);

        // Marcar el módulo internamente como inicializado y corriendo
        initialized_    = true;
        running_        = true;

        return initialized_;    // <- true
    }

    bool isInitialized() const {
        return initialized_;
    }

    void loadConfig(void* config) {

        if (!config)
            return;

        SYS_INFO("TTSMgr","Reading config node...");

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Cargar TTSPlayers a partir del json
        // #TODO

        SYS_INFO("TTSMgr","Config node read OK");
    }

    void cerrar() {

        if (!running_)
            return;

        // Notifica el estado de cerrado (para threads, etc.)
        running_ = false;

        // Cierra el núcleo de TTS
        SYS_INFO("TTSMgr","Closing ttsCore...");
        ttsCore_.cerrar();

        // Espera a que se cierren los hilos
        SYS_INFO("TTSMgr","Waiting for running threads...");
        if (hilo_consumer_.joinable())
            hilo_consumer_.join();

        if (hilo_ttscore_.joinable())
            hilo_ttscore_.join();

        SYS_INFO("TTSMgr","TTS closed successfuly");
    }

    void Ejecutar() {

        // Añadir paquete a la queue
        /* #TODO */


        // Avisar al worker de que hay paquete
        queue_cv_.notify_all();
    }

    
// TTSCore ------------------------------------------------------------------------------

    bool generateWav(std::string const& modelName, std::string const& text, std::string wavname) {
        return ttsCore_.generateWav(modelName, text, wavname);
    }

    std::vector<std::string> getAvailableModels() {
        return ttsCore_.getAvailableModels();
    }

    std::vector<std::string> getLoadedModels() const {
        return ttsCore_.getLoadedModels();
    }

    short numLoadedModels() const {
        return ttsCore_.numLoadedModels();
    }

    short numAvailableModels() const {
        return ttsCore_.numAvailableModels();
    }


// Gestión de reproductores TTS ---------------------------------------------------------

    bool add_tts_player(std::string const& name) {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        
        if (ttsPlayers_.find(name) != ttsPlayers_.end()) {
            return false; // Ya existe un reproductor para ese canal/dispositivo
        }

        // Creamos el reproductor dedicado
        std::unique_ptr<TTSPlayer> player = std::make_unique<TTSPlayer>();
        player->init(name);

        // INYECCIÓN: Generar audio del texto usando TTSCore
        player->setTTSCallback([this](std::string const& modelName, std::string const& text) -> std::vector<float> {
            AudioData data = ttsCore_.generate(modelName, text);
            return data.samples;
        });

        // INYECCIÓN: Reproducir el audio usando SoundMgr
        player->setPlaybackCallback([this](std::vector<float>& audio, const std::string& playbackName) -> bool {
            if (!snd_) {
                SYS_WARN("TTSPlayer","Cannot reproduce audio: Sound module not defined");
                return false;
            }

            // #TODO
        });

        ttsPlayers_[name] = std::move(player);
        return true;
    }

    bool remove_tts_player(std::string const& name) {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        return ttsPlayers_.erase(name) > 0;
    }


// TTSPlayer ----------------------------------------------------------------------------

    bool play(std::string const& text) {
        // #TODO
    }


// Hilos --------------------------------------------------------------------------------

    void TWorker() {

        while (running_) {

            // Salir si el programa se está cerrando (antes de bloqueo)
            if (!running_)
                break;

            // Forzar la espera hasta que sea notificado de un paquete nuevo
            std::unique_lock<std::mutex> lock(queue_mtx_);
            queue_cv_.wait(lock, [this] {
                return !running_ || !queue_.empty();
            });

            // Salir si el programa se está cerrando (después de bloqueo)
            if (!running_)
                break;

            // Va consumiendo la cola de datos pasándoselo a los ttsPlayers
            /* #TODO */

            // Lógica de ejemplo (borrar al implementar la de verdad)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        }
    }


// iComm --------------------------------------------------------------------------------

// #TODO


private:

/************ Variables ********************************************************/

    using TTSPlayers = std::unordered_map<std::string, std::unique_ptr<TTSPlayer>>;
    
    // Ejecución
    std::atomic<bool>           running_;       ///< flag de aplicación corriendo (para hilos)
    bool                        initialized_;   ///< Bandera para indicar inicialización exitosa
    std::thread                 hilo_ttscore_;  ///< Hilo inicializador de TTSCore

    // Cola de datos
    std::thread                 hilo_consumer_; ///< Hilo consumidor de datos TTS (queue)
    std::queue<TTSDataPacket>   queue_;         ///< Cola de comandos
    std::mutex                  queue_mtx_;     ///< Mutex de cola de comandos
    std::condition_variable     queue_cv_;      ///< Conditional variable para mutex de cola

    // Módulos
    SoundMgr*                   snd_;           ///< Puntero a clase de gestión de audio
    TTSCore                     ttsCore_;       ///< Clase núcleo de tts

    // Reproductores TTS (usan playback de soundmgr)
    TTSPlayers                  ttsPlayers_;    ///< Lista de reproductores TTS

};
