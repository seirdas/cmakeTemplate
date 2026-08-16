#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>


/**
 * @class AudioPlaybackModule
 * @brief Clase para la reproducción de audio utilizando miniaudio.
 */
class AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioPlaybackModule.
     * @param ctx (ma_context*) Contexto de mini audio.
     * @param device_info (ma_device_info*) Información del dispositivo de audio.
     */
    AudioPlaybackModule(void* ctx, const void* device_info);

    /**
     * @brief Destructor de AudioPlaybackModule.
     * @note virtual para permitir poliformismo y 
     *  destrucción segura en clases derivadas
     */
    virtual ~AudioPlaybackModule();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa el motor de audio para la reproducción
     * @note virtual para poder "reemplazar" y sobreescribir el método y sus parámetros en clases derivadas
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @param playbackName Nombre asignado a este módulo
     * @return true si se inicia correctamente, false en caso contrario.
     */
    bool init(
        void*               config          = nullptr, 
        std::string const&  playbackName    = ""
    );

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);

    /**
     * @brief Detiene la reproducción de audio y libera recursos.
     */
    bool close();

    /**
     * @brief Desinicializa y cierra el módulo 
     *  y lo vuelve a inicializar con los nuevos parámetros
     */
    bool reload();


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
     * @brief Detiene la reproduccitón de un sonido con el ID especificado.
     * @param audioName  Nombre del sonido a detener.
     * @param force      Forzar la parada, independientemente
     *  del valor de forceStop de la instancia del sonido reproduciéndose.
     * @param fadeOutMs  Parar bajando el volumen progresivamente durante x ms (0 = desactivado).
     * @param pitchOutMs Parar bajando el pitch progresivamente durante x ms (0 = desactivado).
     */
    void stopAudio(
        std::string const& audioName, 
        bool force = false,
        unsigned int fadeOutMs = 0,
        unsigned int pitchOutMs = 0
    );

    /**
     * @brief Establece el volumen de un sonido en ejecución.
     * @param audioName Nombre del sonido a detener.
     * @param volume Volumen del sonido (0 a 100).
     */
    void setVolume(std::string const& audioName, unsigned short volume);

    /**
     * @brief Establece el volumen global del módulo de reproducción (global).
     * @note Actualiza los sonidos que se están reproduciendo.
     * @note Se limita el máximo a 100
     * @param volume Volumen del sonido (0 a 100).
     */
    void setModuleVolume(unsigned short volume);

    /**
     * @brief Establece el tono de un sonido en ejecución.
     * @param audioName Nombre del sonido a detener.
     * @param pitch Tono del sonido.
     */
    void setPitch(std::string const& audioName, float pitch);


// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Devuelve el nombre del dispositivo
     * @return Nombre del dispositivo
     */
    std::string getDeviceName() const;

    /**
     * @brief Obtiene el nombre de este AudioInputModule
     * @return Nombre de este componente
     */
    std::string getModuleName() const;

    /**
     * @brief Devuelve el volumen global del módulo
     * @return Volumen (0-100)
     */
    unsigned short getModuleVolume() const;

    /**
     * @brief Verifica si un audio está en reproducción
     * @param audioName Nombre del audio
     * @return @c true si el sonido está en reproducción, @c false en caso contrario.
     */
    bool isPlaying(std::string const& audioName) const;

    /**
    * @brief Comprueba si el módulo tiene algún sonido activo reproduciéndose.
    * @return true si hay al menos un sonido sin terminar (ocupado), false si está libre.
    */
    bool isBusy() const;

    /**
     * @brief Devuelve el nombre del dispositivo de audio.
     * @return Nombre del dispositivo.
     */
    std::string deviceName() const;


    // #TODO Añadir método: Cuando se añadan varios dispositivos de reproducción,
    // obtener un dispositivo libre que no esté reproduciendo (el siguiente, por ejemplo)


// #TODO revisar qué cosas deberían ser private y protected
protected:

// Caché --------------------------------------------------------------------------------

    /**
     * @brief Precarga un archivo de audio en la caché.
     * @param filepath Ruta del archivo de audio.
     * @return true si la precarga tiene éxito, false en caso contrario.
     */
    bool preload_audio_file(const std::string& filepath);


// Limpieza de sonidos ------------------------------------------------------------------

    /**
     * @brief Comprueba los sonidos que han terminado y los desinicializa 
     * @note Esta función está diseñada para correr en un hilo independiente
     */
    void t_cleanup();

    /**
     * @brief Marca un sonido como que ha terminado de reproducirse 
     * @details Mueve la instancia de sonido a una cola de limpieza, la cual
     *   se desinicializarán en diferido por un hilo independiente.
     * @param sound sonido (ma_sound)
     */
    void stop_and_send_to_cleanup(void* sound);

    /**
     * @brief Libera de la memoria los sonidos de la lista de limpieza
     */
    void cleanup_sounds();


// Limpieza de caché de audios ----------------------------------------------------------

    /**
     * @brief Hilo de trabajo para la limpieza y descarga diferida de audios en caché.
     * @details Revisa de forma periódica el tiempo de inactividad de los audios cargados
     *  en la caché. Si un audio supera el tiempo de vida especificado y no se
     *  encuentra actualmente en reproducción, libera el audio de la memoria.
     * @note Esta función está diseñada para ejecutarse de manera continua
     *  en un hilo independiente.
     */
    void t_cache_reaper();


// Thread de pitchOut -------------------------------------------------------------------

    /**
     * @brief Inicia un hilo asíncrono para reducir progresivamente el volumen de un sonido.
     * @param audioName Nombre identificador del audio.
     * @param soundPtr (ma_sound*) Puntero a la estructura ma_sound de miniaudio.
     * @param totalTransitionMs Tiempo total de la transición en milisegundos.
     */
    void start_fadeout_thread(
        std::string const&  audioName,
        void*               soundPtr,
        unsigned int        totalTransitionMs
    );

    /**
     * @brief Inicia un hilo asíncrono para reducir progresivamente el pitch de un sonido.
     * @param audioName Nombre identificador del audio.
     * @param soundPtr (ma_sound*) Puntero a la estructura ma_sound de miniaudio.
     * @param totalTransitionMs Tiempo total de la transición en milisegundos.
     * @param cleanup Indica si se ha de gestionar la limpieza del audio o no.
     */
    void start_pitchout_thread(
        std::string const& audioName,
        void* soundPtr,
        unsigned int totalTransitionMs,
        float startPitch,
        bool cleanup
    );


/************ Variables ****************************************************************/

// Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;                 ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                    initialized_;           ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>       running_;               ///< flag de aplicación corriendo (para hilos)
    std::string             name_;                  ///< Nombre del módulo
    unsigned short          globalVol_;             ///< Volumen global del módulo (0-100)

// Listas de sonidos
    mutable std::mutex  playing_sounds_mtx_;        ///< Mutex para el mapa de sonidos
    mutable std::mutex  sounds_cache_mtx_;          ///< Mutex para la lista de caché de sonidos
    // SoundList        playing_sounds;              // variable en PIMPL
    // CacheList        sounds_cache;                // variable en PIMPL

// Limpieza de sonidos
    /* Todo esto es necesario porque no se pueden hacer uninit de los audios según terminan, hay que hacerlo diferido */
    std::thread             cleanup_thread_;        ///< Hilo de limpieza de sonidos en reproducción (cuando terminan)
    std::condition_variable cleanup_cv_;            ///< Variable de condición para despertar al hilo
    std::mutex              cleanup_mtx_;           ///< Mutex para la cola de limpieza
    // std::queue<ma_sound*>   cleanup_queue;       // variable en PIMPL

// Limpieza de caché de sonidos por tiempo
    std::thread             cachereaper_thread_;    ///< Hilo para descargar sonidos cargados respecto a su timeout
    std::mutex              cachereaper_mtx_;       ///< Mutex para el acceso al mapa last_used
    std::condition_variable cachereaper_cv_;        ///< Condition variable para el acceso al hilo reaper
    std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida en segundos de los audios en caché

// Hilos pitchout
    std::atomic<size_t> active_fadeouts_threads_;   ///< Número de hilos pitchout ejecutándose
    std::condition_variable pitch_threads_cv_;      ///< CV para esperar a que cierren los hilos pitchout en cierre

};
