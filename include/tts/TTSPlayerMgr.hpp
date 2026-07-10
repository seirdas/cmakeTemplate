#pragma once

#include <memory>   // unique_ptr
#include <string>

/* temp para que funcione en el hpp */
#include "tts/TTSPlayerMgr.hpp"
#include "tts/TTSPlayer.hpp"
#include "tts/TTSData.hpp"
#include "tts/TTSCore.hpp"

#include "Sound/SoundMgr.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
/************************************/


// Declaración implícita
// class TTSCore;
// class SoundMgr;


class TTSPlayerMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayerMgr(TTSCore& tts, SoundMgr& snd) :
        tts_(tts) 
    {

    }
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayerMgr() {

    }


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config) {

    }

    void loadConfig(void* config) {

    }

    void Ejecutar() {

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

        // INYECCIÓN DE DEPENDENCIAS (DIP): Le pasamos las lambdas al Player
        // 1. Cómo generar el audio usando TTSCore
        player->setTTSCallback([this](std::string const& modelName, std::string const& text) -> std::vector<float> {
            AudioData data = tts_.generate(modelName, text);
            return data.samples;
        });

        // 2. Cómo reproducir el audio usando SoundMgr
        player->setPlaybackCallback([this](std::vector<float>& audio, const std::string& playbackName) -> bool {
            
            // #TODO
        });

        ttsPlayers_[name] = std::move(player);
        return true;
    }

    bool remove_tts_player(std::string name) {

    }


// iComm --------------------------------------------------------------------------------




private:

    using TTSPlayers = std::unordered_map<std::string, std::unique_ptr<TTSPlayer>>;

    // Reproductores TTS
    TTSPlayers              ttsPlayers_;    ///< Lista de reproductores TTS
    
    // Cola de datos
    std::queue<TTSData>     queue_;         ///< Cola de comandos
    std::mutex              queue_mtx_;     ///< Mutex de cola de comandos
    std::condition_variable queue_cv_;      ///< Conditional variable para mutex de cola

    // TTS
    TTSCore&                tts_;           ///< Clase núcleo de tts

};
