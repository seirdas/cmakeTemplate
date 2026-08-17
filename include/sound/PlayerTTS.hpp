#pragma once

#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "sound/AudioPlaybackModule.hpp"


class PlayerTTS : public AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor del módulo.
     *  Utiliza el constructor de la clase padre
     * @param ctx (ma_context*) Contexto de mini audio.
     * @param device_info (ma_device_info*) Información del dispositivo de audio.
     */
    PlayerTTS(std::string const& moduleName, void* ctx, const void* device_info);
    
    /**
     * @brief Destructor del módulo
     *  Utiliza el destructor de la clase padre
     */
    ~PlayerTTS() override = default;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief 
     * @param modelName 
     * @param text 
     * @return 
     */
    bool playTTS(std::string const& modelName, std::string const& text);


// Inyección de función texto a audio ---------------------------------------------------

    /**
     * @brief Función inyectada que utiliza el núcleo de tts para pasar un texto a audio
     * @param modelName Nombre del modelo tts a usar
     * @param text Texto para convertir a audio
     * @returns std::vector<float> Vector de muestras de audio en formato float
     */
    using TTSFunction = std::function<std::vector<float>(std::string const& modelName, std::string const& text)>;

    /**
     * @brief Registra un callback externo para procesar un texto y devolver una trama de datos de audio.
     * @param fn Función callback inyectada: std::vector<float> func(std::string const& modelName, std::string const& text)
     */
    void setCallback_onTextToAudio(TTSFunction fn);

    /**
     * @brief Elimina la función asociada a onTextToAudio
     */
    void clearCallback_onTextToAudio();

    /**
     * @brief Indica si este módulo tiene una función inyectada para
     *  convertir texto a audio (módulo TTS)
     * @return @c true si tiene la función inyectada, @c false en caso contrario (nullptr) 
     */
    bool hasCallback_onTextToAudio();


private:

// Hilos --------------------------------------------------------------------------------

    void t_data_consumer();


// Procesado de elementos de la cola ----------------------------------------------------

    // Declaración anticipada
    struct queueElement;
    bool reproducir_elemento(queueElement element);


/************ Variables ********************************************************/

// Funciones inyectadas
    TTSFunction             onTextToAudio_cb_;         ///< Función inyectada para pasar de texto a audio
    std::mutex              onTextToAudio_mtx_;        ///< Mutex para onTextToAudio

// Datos
    std::string             playbackName_;          ///< Nombre del playback por el que se reproduce
    std::string             texto_en_proceso_;      ///< Texto que está procesando (generando->reproduciendo) el módulo

// Cola de textos a procesar

    /** @brief Elemento de la cola de textos */
    struct queueElement {
        std::string text;       ///< Texto a reproducir
        std::string modelName;  ///< Nombre del modelo de voz TTS a usar
    };
    std::queue<queueElement>    cola_textos_;       ///< Cola de textos pendientes de reproducir
    std::mutex                  cola_textos_mtx_;   ///< Mutex para cola de textos
    std::condition_variable     cola_textos_cv_;    ///< Condition variable para cola de textos
    std::thread                 data_consumer_thread_;     ///< Hilo dedicado a procesar la cola de textos

};
