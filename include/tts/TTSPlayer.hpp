#pragma once

#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>


class TTSCore;

class TTSPlayer {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer(std::string const& playerName);
    
    /**
     * @brief Destructor 
     */
    ~TTSPlayer();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa el reproductor TTSPlayer
     * @param pbName Nombre del reproductor playback
     *  gestionado por SoundMgr que se usa para reproducir audio
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(std::string const& pbName);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
     * @brief Cierra los procesos activos y el hilo de 
     *  procesamiento de paquetes
     * @return @c true cuando se ha cerrado correctamente, @c false en caso contrario.
     */
    bool close();

    /**
     * @brief 
     * @param modelName 
     * @param text 
     * @return 
     */
    bool play(std::string const& modelName, std::string const& text);


// Datos --------------------------------------------------------------------------------

    /**
     * @brief Establece el nombre del reproductor playback
     *  gestionado por SoundMgr que se usa para reproducir audio
     * @param pbName 
     */
    void setPlayback(std::string const& pbName);

    /**
     * @brief Devuelve el nombre de este TTSPlayer
     * @return Nombre del TTSPlayer
     */
    std::string getName();

    /**
     * @brief Devuelve el nombre del reproductor playback 
     *  gestionado por SoundMgr que se usa para reproducir audio
     * @return Nombre del playback usado
     */
    std::string getPlaybackName() const;

    /**
     * @brief Devuelve si está generando/reproduciendo un texto
     * @details El truco es que si hay un texto en proceso, está ocupado
     * @return @c true si está procesando, @c false en caso contrario
     */
    bool isBusy();


// Inyección de funciones ---------------------------------------------------------------

    /**
     * @brief Función inyectada que utiliza el núcleo de tts para pasar un texto a audio
     * @param modelName Nombre del modelo tts a usar
     * @param text Texto para convertir a audio
     * @returns std::vector<float> Vector de muestras de audio en formato float
     */
    using TTSFunction = std::function<std::vector<float>(std::string const& modelName, std::string const& text)>;

    void setTTSCallback(TTSFunction fn);

    /**
     * @brief Función inyectada que utiliza el soundMgr para reproducir un audio
     * @param audio Audio a reproducir (obtenido del tts)
     * @param playbackName Nombre del dispositivo playback por el que reproducir el audio
     */
    using PlaybackFunction = std::function<bool(std::vector<float>& audio, std::string const& playbackName)>;

    void setPlaybackCallback(PlaybackFunction fn);


// Hilos --------------------------------------------------------------------------------

    void TProcesarCola();


private:

// Procesado de elementos de la cola ----------------------------------------------------

    // Declaración anticipada
    struct queueElement;
    bool reproducirElemento(queueElement element);


/************ Variables ********************************************************/

// Funciones inyectadas
    TTSFunction             fn_textToAudio;         ///< Función inyectada para pasar de texto a audio
    PlaybackFunction        fn_audioToPlayback;     ///< Función inyectada para reproducir audio en playback

// Inicialización y ejecución
    bool                    initialized_;           ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>       running_;               ///< Flag de módulo corriendo (para hilos)

// Datos
    std::string const       name_;                  ///< Nombre asignado a este TTSPlayer
    std::string             playbackName_;          ///< Nombre del playback por el que se reproduce
    std::string             texto_en_proceso_;      ///< Texto que está procesando (generando->reproduciendo) el módulo

// Cola de textos a procesar

    /**
     * @brief Elemento de la cola de textos
     */
    struct queueElement {
        std::string text;       ///< Texto a reproducir
        std::string modelName;  ///< Nombre del modelo de voz TTS a usar
    };
    std::queue<queueElement>    cola_textos_;       ///< Cola de textos pendientes de reproducir
    std::mutex                  cola_textos_mtx_;   ///< Mutex para cola de textos
    std::condition_variable     cola_textos_cv_;    ///< Condition variable para cola de textos
    std::thread                 worker_thread_;     ///< Hilo dedicado a procesar la cola de textos

};
