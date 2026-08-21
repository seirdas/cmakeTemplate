#pragma once

#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "sound/AudioPlaybackModule.hpp"
#include "sound/TTSCore.hpp"          // Para el tipo AudioData (samples + sample_rate)


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
     * @brief Destructor del módulo.
     *  Para y une el hilo consumidor de textos antes de liberar los
     *  recursos de audio de la clase padre.
     */
    ~PlayerTTS() override;


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa el módulo (motor de audio de la clase padre) y arranca
     *  el hilo consumidor de textos encargado de generar y reproducir el TTS.
     * @param config Datos de configuración (diseñado para recibir un puntero a json).
     * @param playbackName Nombre asignado a este módulo.
     * @return true si se ha iniciado correctamente, false en caso contrario.
     */
    bool init(void* config = nullptr, std::string const& playbackName = "") override;

    /**
     * @brief Detiene el hilo consumidor de textos y cierra el módulo
     *  (motor de audio) de la clase padre.
     */
    bool close() override;

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    PlayerTTS(PlayerTTS const&) = delete;
    PlayerTTS& operator=(PlayerTTS const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    PlayerTTS(PlayerTTS&&) = delete;
    PlayerTTS& operator=(PlayerTTS&&) = delete;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Encola un texto para ser convertido a audio (con el modelo indicado)
     *  y reproducido. El hilo consumidor procesa la cola en orden de llegada.
     * @details El audio generado se gestiona igual que un audio de archivo o un
     *  morse (identificado por @p audioName), por lo que se puede usar ese nombre
     *  con las funciones heredadas de AudioPlaybackModule (setVolume, setPitch, stop...).
     * @param modelName Nombre del modelo tts a usar
     * @param text Texto a reproducir
     * @param audioName Nombre con el que se identifica este sonido una vez generado.
     * @return true si el texto se ha encolado correctamente, false en caso contrario.
     */
    bool playTTS(std::string const& modelName, std::string const& text, std::string const& audioName);


// Inyección de función texto a audio ---------------------------------------------------

    /**
     * @brief Función inyectada que utiliza el núcleo de tts para pasar un texto a audio
     * @param modelName Nombre del modelo tts a usar
     * @param text Texto para convertir a audio
     * @returns AudioData Muestras de audio generadas junto con su sample rate real
     */
    using TTSFunction = std::function<AudioData(std::string const& modelName, std::string const& text)>;

    /**
     * @brief Registra un callback externo para procesar un texto y devolver una trama de datos de audio.
     * @param fn Función callback inyectada: AudioData func(std::string const& modelName, std::string const& text)
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
        std::string audioName;  ///< Nombre con el que se identifica este sonido una vez generado
    };
    std::queue<queueElement>    cola_textos_;       ///< Cola de textos pendientes de reproducir
    std::mutex                  cola_textos_mtx_;   ///< Mutex para cola de textos
    std::condition_variable     cola_textos_cv_;    ///< Condition variable para cola de textos
    std::thread                 data_consumer_thread_;     ///< Hilo dedicado a procesar la cola de textos

};
