
#include "tts/TTSPlayerMgr.hpp"
#include "tts/TTSPlayer.hpp"
#include "tts/TTSData.hpp"

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


TTSPlayerMgr::TTSPlayerMgr() :
    pimpl_(std::make_unique<Impl>()) 
{

}
