#pragma once


#include <string>
#include <memory>
#include <unordered_map>

class TTSPlayer;



class TTSPlayerMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayerMgr();
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayerMgr();


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config);

    void loadConfig(void* config);


private:

    std::unordered_map<std::string, std::unique_ptr<TTSPlayer>>   ttsPlayers_;

};
