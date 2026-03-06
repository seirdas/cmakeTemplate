#pragma once

#include <atomic>
#include <string>
#include <miniaudio.h>


class AudioInputModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor.
     */
    AudioInputModule(ma_context* ctx);

    /**
     * @brief Destructor.
     */
    ~AudioInputModule();


// Grabación ----------------------------------------------------------------------------

    /**
     * @brief Inicia la grabación en un archivo wav.
     * @param filename Nombre del archivo (sin extensión).
     */
    bool StartRec(std::string const& filename);

    /**
     * @brief Para la grabación.
     * @return true si la parada ha sido correcta, false en caso contrario.
     */
    bool StopRec();

    /**
     * @brief Indica si está grabando actualmente.
     * @return True si está grabando, false en caso contrario.
     */
    bool isRecording();


private:
    
    // Métodos para grabación
    /**
    * @brief Inicialización de encoder de grabación a wav
    */
    bool initWavEncoder();

    /**
     * @brief Configuración de dispositivo de grabación
     */
    bool initRecorder();

    /**
     * @brief Callback de captura/grabación.
     * @details Se llama constantemente mientras escribe.
     * @param device Dispositivo de grabación.
     * @param output Salida.
     * @param input Entrada.
     * @param frameCount Contador de muestras.
     */
    static void dataCallback(
        ma_device*  device, 
        void*       output, 
        const void* input, 
        ma_uint32   frameCount);
    
        
    /************ Variables ********************************************************/

    // Motor de audio
    ma_context* snd_ctx_;
    ma_device device_;

    // Motor de grabación
    struct RecordingContext {                               // Estructura de datos de grabación
        std::string             filename        = "";       // Nombre del archivo a grabar (.wav)
        ma_encoder              encoder         = {};       // Encoder.
        std::atomic<uint64_t>   framesWritten   = 0;        // Muestras escritas en el archivo.
        uint64_t                maxFrames       = 0;        // Máximo de muestras a escribir en el archivo.
        std::atomic<bool>       recording       = false;    // Flag que indica si está grabando.
    };

    RecordingContext    rec_ctx_        = {};               // Estructura de contexto/datos de grabación
    ma_uint32           sampleRate      = 44100;            // Frecuencia de muestreo de grabación.
    ma_uint32           channels        = 2;                // Canales de grabación.
    ma_uint32           secondsToRecord = 60*60;            // Segundos máximos de grabación (1h por defecto).
};