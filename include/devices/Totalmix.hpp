#pragma once

/**
 * @class TotalMixClient
 * @brief Controla el driver TotalMixFX via OSC/UDP.
 * @note  Configuración de TotalMix: Options → Enable OSC Control y "Submix linked to OSC Control".
 */
class Totalmix {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    Totalmix();
    
    /**
     * @brief Destructor estándar 
     */
    ~Totalmix();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicialización de la conexión al driver Totalmix 
     */
    bool init();


// Métodos de control -------------------------------------------------------------------

    // #TODO
    
    void setOnePlayback(int numOut, int numPb,  float vol);
    void setOneInput   (int numOut, int numIn,  float vol);
    void setOneOutput  (int numOut,              float vol);

    void setMuteInput   (int numIn,  bool mute);
    void setMuteOutput  (int numOut, bool mute);
    void setMutePlayback(int numPb,  bool mute);



private:

    
/************ Variables ********************************************************/


};
