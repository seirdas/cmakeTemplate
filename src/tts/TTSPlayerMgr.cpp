
#include "tts/TTSPlayerMgr.hpp"
#include "tts/TTSPlayer.hpp"
#include "tts/TTSData.hpp"
#include "tts/TTSCore.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>


struct TTSPlayerMgr::Impl {
    std::unordered_map<std::string, TTSPlayer>   ttsPlayers_;
    
    std::queue<TTSData>     queue_;
    std::mutex              queue_mtx_;
    std::condition_variable queue_cv_;
};

// General ------------------------------------------------------------------------------

TTSPlayerMgr::TTSPlayerMgr(TTSCore& tts) :
    pimpl_(std::make_unique<Impl>()),
    tts_(tts)
{

}

TTSPlayerMgr::~TTSPlayerMgr() {

}


// Ejecución ----------------------------------------------------------------------------

bool TTSPlayerMgr::init(void* config) {

}

void TTSPlayerMgr::loadConfig(void* config) {

}

void TTSPlayerMgr::Ejecutar() {

}


// Gestión de reproductores TTS ---------------------------------------------------------

bool TTSPlayerMgr::add_tts_player(std::string name) {

}

bool TTSPlayerMgr::remove_tts_player(std::string name) {

}
