#pragma once


#include "sound/AudioPlaybackModule.hpp"
#include <memory>


class PlayerAudio : public AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor del módulo.
     *  Utiliza el constructor de la clase padre
     * @param ctx (ma_context*) Contexto de mini audio.
     * @param device_info (ma_device_info*) Información del dispositivo de audio.
     */
    PlayerAudio(std::string const& moduleName, void* ctx, const void* device_info);

    /**
     * @brief Destructor del módulo
     *  Utiliza el destructor de la clase padre
     */
    ~PlayerAudio() override = default;

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    PlayerAudio(PlayerAudio const&) = delete;
    PlayerAudio& operator=(PlayerAudio const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
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
    bool init(
        void*               config          = nullptr, 
        std::string const&  playbackName    = ""
    ) override;

    /**
     * @brief Añade a la liberación de recursos las cosas específicas de este módulo
     * @details Por ejemplo, cerrar el hilo reaper de caché de audios. 
     */
    bool close() override;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Reproduce un archivo de audio con las configuraciones especificadas.
     * @param filepath Ruta del archivo de audio.
     * @param volume Volumen del sonido (0 a 100)
     * @param loop Modo de repetición
     * @param forceStop Fuerza la parada si se desactiva (de lo contrario, deja terminar el wav)
     * @param pitch Tono del sonido (0.0f - )
     * @return Un identificador único para la instancia del sonido
     */
    void playAudio(
        const std::string&  filepath,
        unsigned short      volume = 100,
        bool                loop = false,
        bool                forceStop = false,
        unsigned short      pitch = 1
    );

    /**
     * @brief Reproduce un archivo de audio buscándolo por nombre dentro de la carpeta
     *  configurada para este playback (ver @p audioFolder del constructor).
     * @param filename Nombre del archivo (ej. "ding.wav"), sin ruta.
     * @param volume Volumen del sonido (0 a 100)
     * @param loop Modo de repetición
     * @param forceStop Fuerza la parada si se desactiva (de lo contrario, deja terminar el wav)
     * @param pitch Tono del sonido ( 1.0f = "normal" )
     */
    void playFromFolder(
        std::string const&  filename,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch
    );


// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Establece la ruta de la carpeta de audios de este módulo
     * @param audioFolder Ruta de la carpeta de audios
     */
    void setAudioFolder(std::string const& audioFolder);


private:

// Caché --------------------------------------------------------------------------------
    
    /**
     * @brief Precarga un archivo de audio en la caché.
     * @param filepath Ruta del archivo de audio.
     * @return true si la precarga tiene éxito, false en caso contrario.
     */
    bool preload_audio_on_cache(const std::string& filepath);

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
