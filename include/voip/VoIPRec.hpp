#pragma once

#include <array>
#include <atomic>
#include <string>
#include <vector>
#include <functional>

constexpr int NUM_TX = 8;


// Paquete de datos de audio VoIPRec legacy
struct LegacyAudioPacket {
    unsigned long long  txIds[8];
    unsigned short      coding;
    unsigned short      freq;
    unsigned short      b_s;
    unsigned short      nsec;
    int                 timeStamp;
    unsigned short      spare;
    unsigned short      size;
    short               voice[250];   // _IPSendData = 250 muestras mono, igual que el sistema viejo
};


// Forward declaration para clases Mgr usadas
class NetMgr;
class SoundMgr;


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

/**
 * @class VoIPRec
 * @brief Enlace entre un dispositivo de audio y un socket UDP.
 */
class VoIPRec {
    
public:

// General ------------------------------------------------------------------------------

    using SendFn = std::function<void(const std::vector<char>&)>;

    VoIPRec(const std::string& name, SendFn send_cb);

    ~VoIPRec();

    bool init(void* config);

// Estado -------------------------------------------------------------------------------

    bool start();

    bool stop();


private:

// Paquete de datos de audio ------------------------------------------------------------

    std::vector<char> buildLegacyPacket();


    /************ Variables ********************************************************/

// Aliases
    using listaTX = std::array<unsigned int, NUM_TX>;

// Inicialización y ejecución
    std::atomic<bool>   running_;       ///< Estado del módulo (activo/desactivado)
    
// Conexiones externas
    SoundMgr*       snd_;
    NetMgr*         net_;

// Metadatos del paquete de audio
    std::vector<short>      audioBuffer_;       ///< Buffer que acumula las muestras de audio capturadas (formato s16)
    unsigned int            sampleRateHz_;      ///< Frecuencia de muestreo
    short                   channels_;          ///< Número de canales
    unsigned int            framesPerPacket_;   ///< _IPSendData del legacy
    AudioEncoding           encoding_;          ///< Codificación de envío
    float                   voxThreshold_;      ///< Threshold de envío RMS normalizado 0-100
    int                     voxTailFrames_;     ///< paquetes de "silencio de cierre"
    listaTX                 txIds_;             ///< Metadatos de paquete legacy (IDTX)
    unsigned short          nsec_ = 0;          ///< número de secuencia 0-127 (legacy)

};
