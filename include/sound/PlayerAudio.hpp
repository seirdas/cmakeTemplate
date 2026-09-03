#pragma once

#include "sound/AudioPlaybackModule.hpp"


class PlayerAudio : public AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor del módulo.
     *  Utiliza el constructor de la clase padre
     * @param ctx (ma_context*) Contexto de mini audio.
     */
    PlayerAudio(std::string const& moduleName, void* ctx);

    /**
     * @brief Destructor del módulo
     *  Utiliza el destructor de la clase padre
     */
    ~PlayerAudio() override = default;

    // Sin copia ni movimiento
    PlayerAudio(const PlayerAudio&) = delete;
    PlayerAudio& operator=(const PlayerAudio&) = delete;
    PlayerAudio(PlayerAudio&&) = delete;
    PlayerAudio& operator=(PlayerAudio&&) = delete;


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Añade características especiales a la inicialización de este módulo
     * @details Inicializa, por ejemplo, el hilo reaper de caché de audios
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @param playbackName Nombre asignado a este módulo
     * @return true si se inicia correctamente, false en caso contrario.
     */
    bool init(void* config = nullptr) override;

    /**
     * @brief Añade a la liberación de recursos las cosas específicas de este módulo
     * @details Por ejemplo, cerrar el hilo reaper de caché de audios. 
     */
    bool close() override;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Reproduce un archivo de audio con las configuraciones especificadas.
     * @param filepath Ruta del archivo de audio.
     * @param deviceAlias Alias del dispositivo por el que reproducir.
     * @param volume Volumen del sonido (0 a 100).
     * @param loop Modo de repetición.
     * @param forceStop Si activado, fuerza la parada (de lo contrario, deja terminar el audio).
     * @param pitch Tono del sonido (default 1.0f).
     */
    void playAudio(
        const std::string&  filepath,
        const std::string&  deviceAlias = "",
        unsigned short      volume      = 100,
        bool                loop        = false,
        bool                forceStop   = false,
        unsigned short      pitch       = 1
    );


private:

// Caché --------------------------------------------------------------------------------
    
    /**
     * @brief Construye la clave compuesta usada para indexar tanto la caché de
     *  audios precargados como el mapa de sonidos en reproducción.
     * @details La caché está indexada por (alias + filepath), porque un
     *  ma_sound cacheado solo se puede copiar (ma_sound_init_copy) sobre el mismo
     *  engine con el que se cargó, y cada device tiene su propio ma_engine.
     * @param deviceAlias Alias del dispositivo de playback (ver addPlaybackDevice).
     * @param filepath Ruta del archivo de audio.
     * @return Clave compuesta con el formato "alias|filepath".
     */
    std::string make_cache_key(const std::string& deviceAlias, const std::string& filepath);

    /**
     * @brief Precarga un archivo de audio en la caché.
     * @param filepath Ruta del archivo de audio.
     * @param deviceAlias Alias del dispositivo por el que reproducir.
     * @return true si la precarga tiene éxito, false en caso contrario.
     */
    bool preload_audio_on_cache(
        const std::string& filepath, 
        const std::string& deviceAlias);

    /**
     * @brief Hilo de trabajo para la limpieza y descarga diferida de audios en caché.
     * @details Revisa de forma periódica el tiempo de inactividad de los audios cargados
     *  en la caché. Si un audio supera el tiempo de vida especificado y no se
     *  encuentra actualmente en reproducción, libera el audio de la memoria.
     * @note Esta función está diseñada para ejecutarse de manera continua
     *  en un hilo independiente.
     */
    void t_cache_reaper();


/************ Variables ****************************************************************/

// Gestión de archivos de audio
    std::string             audioFolder_;           ///< Carpeta de archivos de audio de este playback (usada por playFromFolder)

// Limpieza de caché de sonidos por tiempo
    std::thread             cachereaper_thread_;    ///< Hilo para descargar sonidos cargados respecto a su timeout
    std::mutex              cachereaper_mtx_;       ///< Mutex para el acceso al mapa last_used
    std::condition_variable cachereaper_cv_;        ///< Condition variable para el acceso al hilo reaper
    std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida en segundos de los audios en caché

};
