//creo la clase de los inputs
#pragma once

#include <asio.hpp>             // asio external lib
#include <string>               // std::string
#include <vector>               // Vectores
#include <thread>               // Hilos
#include <memory>               // unique_ptr
#include <miniaudio.h>

/**
 * @class AudioInputModule
 * @brief Clase para la captura de audio utilizando mini audio.
 */
class AudioInputModule{

public: 

    // General --------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioInputModule.
     * @param ctx Contexto de mini audio.
     * @param device_info Información del dispositivo de audio.
     */
    AudioInputModule(ma_context* ctx, ma_device_info const& device_info);

    /**
     * @brief Destructor de AudioInputModule.
     */
    ~AudioInputModule();


    //Ejecución ------------------------------------------------------------------------- 

    /**
     * @brief Inicializa el dispositivo de captura
     * @return true si arranca bien, false si no
     */
    bool init();

    /**
     * @brief para de capturar y lo elimina
     */
    void stop();


    // Init -----------------------------------------------------------------------------

    /**
     * @brief Orden de empieza a grabar
     * @return true si consigue grabar, false si falla
     */
    bool StartRec();

    /**
     * @brief Orden de para de grabar
     * @return 
     */
    bool StopRec();

    /** 
     * @brief Está grabando
     */
    bool isRecording();


    // Dispositivos ---------------------------------------------------------------------

    /**
     * @brief devuelve el nombre del dispositivo
     * @return nombre del dispositivo
     */
    std::string deviceName() const { return deviceName_; }


    // Grabacion ------------------------------------------------------------------------

    /**
     * @brief Inicializa el codificador para pasar a .wav
     * @param filename Recibe el nombre del archivo donde se va a guardar el audio
     */
    void InitwavEncoder(const std::string& filename);

    /**
     * @brief  Desinicializa el codificador si eliminamos la entrada 
    */
    void RemovewavEncoder();

    /**
     * @brief Guarda el sonido en un .wav
     */
    void saveSound();

    /**
     * @brief Obtener tamaño del buffer de captura
     * @return tamaño de buffer de captura
     */
    size_t getBufferSize();

    /**
     * @brief Obtener tamaño del buffer de grabación
     * @return tamaño de buffer de grabación
     */
    size_t getRecBufferSize();

    /**
     * @brief booleano para saber si el dispositivo sigue activo o no 
     */
    bool isValid();


    // Nivel RMS ------------------------------------------------------------------------

    /**
     * @brief obtiene el valor del rms
     */
    float getRmsLevel() const { return rmsLevel_; }


private:

    // CallBacks ------------------------------------------------------------------------

    /** 
     * @brief Miniaudio llama a esta función automáticamente cada vez que el micrófono captura un trozo de audio
     */
    static void dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    
    /**
     * @brief Miniaudio llama a esta función cuando el estado del dispositivo cambia 
     */
    static void notificationCallback_(const ma_device_notification* pNotification);


    /************ Variables ********************************************************/

    // Configuración de entrada
    ma_context*             ctx_;                           ///< Contexto de audio
    ma_uint32               channels_ = 2;                  ///< Canales del audio
    ma_uint32               sampleRate_ = 44100;            ///< Frecuencia de muestreo
    ma_device               device_;                        ///< Dispositivo
    ma_device_info          device_info_;                   ///< Información del dispositivo
    std::vector<int16_t>    buffer_;                        ///< Buffer que acumula las muestras de audio capturadas (formato s16)
    ma_uint32               processBufferSize_ = 1024;      ///< frames por bloque de proceso

    // Grabación 
    std::vector<int16_t>    rec_buffer_;                    ///< Buffer que acumula las muestras de audio capturadas (formato s16)
    std::atomic<bool>       recording_;                     ///< Flag thread-safe: true = el callback guarda datos, false = los ignora
    ma_encoder              encoder_;                       ///< Codificador de audio para grabación 
    
    // Datos del dispositivo  
    bool                    is_valid_;                      ///< Bandera para indicar si está inicializado el dispositivo
    std::string             deviceName_;                    ///< Nombre del dispositivo

    // Valor de entrada del RMS
    std::atomic<float>      rmsLevel_{0.0f};                ///< guarda el nivel actual de rms
};
