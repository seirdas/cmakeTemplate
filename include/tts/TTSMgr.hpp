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
    TTSMgr(SoundMgr& snd)
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
        SYS_INFO("AppController","Starting TTSCore async load...");
        std::thread tLoadTTS([this, config]() {
                if(!ttsCore_.init(config))
                    SYS_WARN("AppController","TTSCore FAIL");
                else SYS_INFO("AppController","TTSCore OK");

                JsonMgr::instance().update();
            }
        );
        tLoadTTS.detach();  // No necesitamos "esperar" a que termine

        return true;
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

    }

    void cerrar() {
        ttsCore_.cerrar();
    }

    void Ejecutar() {

    }

// TTSCore ------------------------------------------------------------------------------

    bool generateWav(std::string const& modelName, std::string const& text, std::string wavname) {
        return ttsCore_.generateWav(modelName, text, wavname);
    }

    std::vector<std::string> getAvailableModels() {
        return ttsCore_.getAvailableModels();
    }

    std::vector<std::string> getLoadedModels() {
        return ttsCore_.getLoadedModels();
    }

    short numLoadedModels() {
        return ttsCore_.numLoadedModels();
    }

    short numAvailableModels() {
        return ttsCore_.numAvailableModels();
    }




// Gestión de reproductores TTS ---------------------------------------------------------

    bool add_tts_player(std::string name) {
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
            
            // #TODO
        });

        ttsPlayers_[name] = std::move(player);
        return true;
    }

    bool remove_tts_player(std::string name) {

    }


// iComm --------------------------------------------------------------------------------

// #TODO


private:

/************ Variables ********************************************************/

    using TTSPlayers = std::unordered_map<std::string, std::unique_ptr<TTSPlayer>>;
    
    // Cola de datos
    std::queue<TTSData>     queue_;         ///< Cola de comandos
    std::mutex              queue_mtx_;     ///< Mutex de cola de comandos
    std::condition_variable queue_cv_;      ///< Conditional variable para mutex de cola

    // TTS
    TTSCore                 ttsCore_;       ///< Clase núcleo de tts

    // Reproductores TTS (usan playback de soundmgr)
    TTSPlayers              ttsPlayers_;    ///< Lista de reproductores TTS

};
