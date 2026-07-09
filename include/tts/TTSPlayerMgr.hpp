#pragma once

#include <memory>   // unique_ptr
#include <string>

// Declaración implícita
class TTSCore;

class TTSPlayerMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayerMgr(TTSCore& tts);
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayerMgr();


// Ejecución ----------------------------------------------------------------------------

    bool init(void* config);

    void loadConfig(void* config);

    void Ejecutar();


// Gestión de reproductores TTS ---------------------------------------------------------

    bool add_tts_player(std::string name);

    bool remove_tts_player(std::string name);


// iComm --------------------------------------------------------------------------------

    


private:

    // Pointer to implementation
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Clase gestor de TTS
    TTSCore& tts_;

};
