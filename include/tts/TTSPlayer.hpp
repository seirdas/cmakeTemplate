#pragma once



class TTSPlayer {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer();
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayer();


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config);

    void loadConfig(void* config);


private:

    bool    is_busy_;

};
