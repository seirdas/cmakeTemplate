#pragma once

#include <string>

class TTSCore;

class TTSPlayer {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer(TTSCore& ttsCore, std::string AIM_name);
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayer();


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config);

    void loadConfig(void* config);


private:

    bool                is_busy_;

};
