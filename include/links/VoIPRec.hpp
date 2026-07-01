#pragma once

#include <array>

constexpr int NUM_TX = 8;

// Compatibilidad con DIS Signal PDU (IEEE 1278.1-2012, Table B-10)
enum class AudioEncoding {
    MuLaw_8bit          = 1,
    CVSD_MIL_STD_188    = 2,
    ADPCM_G721          = 3,
    PCM_16bit_Signed_BE = 4,        // big-endian — lo que usa DIS por defecto
    PCM_8bit            = 5,
    VQ                  = 6,
    GSM_FullRate        = 8,
    GSM_HalfRate        = 9,
    Speex_Narrowband    = 10,
    PCM_16bit_Signed_LE = 100,      // little-endian — lo que usa el legacy y Windows
    Legacy_sendComs     = 0xFFFF    // Formato legacy (Voiprec)
};

class VoIPRec {
    
public:

// General ------------------------------------------------------------------------------

    VoIPRec();

    ~VoIPRec();

    bool init();

    
private:

    /************ Variables ********************************************************/

    using listaTX = std::array<unsigned int, NUM_TX>;

    // Metadatos del paquete de audio
    unsigned int    sampleRateHz_;      ///< Frecuencia de muestreo
    short           channels_;          ///< Número de canales
    unsigned int    framesPerPacket_;   ///< _IPSendData del legacy
    AudioEncoding   encoding_;          ///< Codificación de envío
    float           voxThreshold_;      ///< Threshold de envío RMS normalizado 0-100
    int             voxTailFrames_;     ///< paquetes de "silencio de cierre"
    listaTX         txIds_;             ///< Metadatos de paquete legacy (IDTX)
};
