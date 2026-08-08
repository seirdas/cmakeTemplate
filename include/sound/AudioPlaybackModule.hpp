#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>


/**
 * @class AudioPlaybackModule
 * @brief Clase para la reproducción de audio utilizando miniaudio.
 */
class AudioPlaybackModule {
    
public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioPlaybackModule.
     * @param ctx Contexto de mini audio.
     * @param device_info Información del dispositivo de audio.
     */
    AudioPlaybackModule(void* ctx, void* const device_info);
    
    /**
     * @brief Destructor de AudioPlaybackModule.
     */
    ~AudioPlaybackModule();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa el motor de audio
     * @return true si se inicia correctamente, false en caso contrario.
     */
    bool init();
    
    /**
     * @brief Detiene la reproducción de audio y libera recursos.
     */
    void stop();


// Acciones -----------------------------------------------------------------------------


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
     * @param id Identificador del sonido a detener.
     */
    void stopAudio(std::string const& audioName, bool force = false);

    /**
     * @brief Establece el volumen de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param volume Volumen del sonido (0.0f a 1.0f).
     */
    void setVolume(std::string const& audioName, float volume);
    
    /**
     * @brief Establece el tono de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param pitch Tono del sonido.
     */
    void setPitch(std::string const& audioName, float pitch);


// Datos del módulo ---------------------------------------------------------------------

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


private:


// Caché --------------------------------------------------------------------------------

    /**
     * @brief Precarga un archivo de audio en la caché.
     * @param filepath Ruta del archivo de audio.
     * @return true si la precarga tiene éxito, false en caso contrario.
     */
    bool preloadAudioFile(const std::string& filepath);


// Limpieza -----------------------------------------------------------------------------

    /**
     * @brief Comprueba los sonidos que han terminado y los desinicializa 
     * @note Esta función está diseñada para correr en un hilo independiente
     */
    void TCleanup();

    /**
     * @brief Marca un sonido como que ha terminado de reproducirse 
     * @details Mueve la instancia de sonido a una cola de limpieza, la cual
     *   se desinicializarán en diferido por un hilo independiente.
     * @param sound sonido (ma_sound)
     */
    void sendToCleanup(void* sound);

    /**
     * @brief Libera de la memoria los sonidos de la lista de limpieza
     */
    void cleanupSounds();


/************ Variables ****************************************************************/
    
// Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;             ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    bool                    initialized_;       ///< Bandera para indicar inicialización exitosa
    std::atomic<bool>       running_;           ///< flag de aplicación corriendo (para hilos)

// Listas de sonidos
    mutable std::mutex  playing_sounds_mtx_;    ///< Mutex para el mapa de sonidos
    mutable std::mutex  sounds_cache_mtx_;      ///< Mutex para la lista de caché de sonidos
    // SoundList       playing_sounds;          // variable en PIMPL
    // CacheList       sounds_cache;            // variable en PIMPL

// Variables de limpieza
    /* Todo esto es necesario porque no se pueden hacer uninit de los audios según terminan, hay que hacerlo diferido*/
    std::thread             cleanup_thread_;    ///< Hilo de limpieza de sonidos en reproducción (cuando terminan)
    std::condition_variable cleanup_cv_;        ///< Variable de condición para despertar al hilo
    // std::queue<ma_sound*>   cleanup_queue;   // variable en PIMPL
    std::mutex              cleanup_mtx_;       ///< Mutex para la cola de limpieza


};
