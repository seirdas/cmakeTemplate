
#pragma once

#include <miniaudio.h>
#include <atomic>
#include <string>

/**
  * @class SoundMgr
  * @brief Gestor de reproducción y grabación de audio basado en miniaudio.
  *  Proporciona inicialización y parada del motor de audio (ma_engine),
  *  y funcionalidades de grabación a WAV con manejo de encoder y dispositivo
  *  de captura. 
  *  La inicialización del motor se controla mediante un flag atómico
  *  para comprobaciones seguras en entornos multihilo.
  *  Notas de diseño:
  *     El callback de captura (dataCallback) es estático y debe ser seguro para
  *  ejecución en hilo de audio; evitar operaciones de E/S que puedan bloquear.
  *     Los flags compartidos usan std::atomic para operaciones atómicas y para
  *  evitar condiciones de carrera entre hilos.
  * @date March 2, 2026
  */
class SoundMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor.
     */
    SoundMgr();

    /**
     * @brief Destructor.
     */
    ~SoundMgr();

    /**
     * @brief Inicialización del motor de audio.
     * @returns 
     */
    bool init();

    /**
    * @brief Para el motor de audio.
    * @returns True Si la parada ha sido correcta, false en caso contrario. 
    */
    bool stop();

    void test();

    void test2();

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
    ma_engine engine_;                              // Motor de audio.
    std::atomic<bool> engine_initialized_;          // Flag para saber si el motor de audio está inicializado.

    // Motor de grabación

    /**
    * @brief Estructura de datos de grabación
    */
    struct RecordingContext {
        std::string             filename;       // Nombre del archivo a grabar (.wav)
        ma_encoder              encoder;        // Encoder.
        std::atomic<uint64_t>   framesWritten;  // Muestras escritas en el archivo.
        uint64_t                maxFrames;      // Máximo de muestras a escribir en el archivo.
        std::atomic<bool>       recording;      // Flag que indica si está grabando.
    };

    RecordingContext    ctx_;                         // Estructura de contexto/datos de grabación
    ma_device           device_;                         
    ma_uint32           sampleRate      = 44100;      // Frecuencia de muestreo de grabación.
    ma_uint32           channels        = 2;          // Canales de grabación.
    ma_uint32           secondsToRecord = 60*60;      // Segundos máximos de grabación (1h por defecto).
};
