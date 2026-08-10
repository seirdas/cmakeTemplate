#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <miniaudio.h>
#include <vector>

/**
 * @enum LoopMode
 * @brief Enum para definir los modos de repetición de sonido.
 */


struct SoundInstance

{
    ma_sound            sound;                      ///< La instancia del sonido en mini audio.
    ma_audio_buffer     buffer;                     ///< Buffer en memoria (solamente si el sonido viene del buffer y no de memoria)

    bool        loopMode   = false; 
    bool        forceStop  = false; 
    bool        finished   = false;            ///< Indica si el sonido ha terminado de reproducirse.
    bool        isbuffer   = false;            ///< Indica si el el buffer está inicializado y hay que liberarlo
};

/**
 * @class AudioPlaybackModule
 * @brief Clase para la reproducción de audio utilizando mini audio.
 */
class AudioPlaybackModule
{
    
public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioPlaybackModule.
     * @param ctx Contexto de mini audio.
     * @param device_info Información del dispositivo de audio.
     */
    AudioPlaybackModule(ma_context* ctx, ma_device_info const& device_info);
    
    /**
     * @brief Destructor de AudioPlaybackModule.
     */
    ~AudioPlaybackModule();


// Acciones -----------------------------------------------------------------------------

    /**
     * @brief Inicializa el motor de audio y comienza la reproducción.
     * @return true si se inicia correctamente, false en caso contrario.
     */
    bool start();
    
    /**
     * @brief Detiene la reproducción de audio y libera recursos.
     */
    void stop();

    /**
     * @brief Precarga un archivo de audio en la caché.
     * @param filepath Ruta del archivo de audio.
     * @return true si la precarga tiene éxito, false en caso contrario.
     */
    bool preload(const std::string& filepath);

    /**
     * @brief Reproduce un archivo de audio con las configuraciones especificadas.
     * @param filepath Ruta del archivo de audio.
     * @param volume Volumen del sonido (0 a 100)
     * @param loop Modo de repetición
     * @param forceStop Fuerza la parada si se desactiva (de lo contrario, deja terminar el wav)
     * @param pitch Tono del sonido (0.0f - )
     * @return Un identificador único para la instancia del sonido
     */
    unsigned long long play(const std::string& filepath,
        unsigned short volume = 100,
        bool loop = false,
        bool forceStop = false,
        unsigned short pitch = 1
    );

    /**
     * @brief Detiene la reproducción de un sonido con el ID especificado.
     * @param id Identificador del sonido a detener.
     */
    void stopSound(unsigned long long id);

    /**
     * @brief Establece el volumen de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param volume Volumen del sonido (0.0f a 1.0f).
     */
    void setVolume(unsigned long long id, float volume);
    
    /**
     * @brief Establece el tono de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param pitch Tono del sonido.
     */
    void setPitch(unsigned long long id, float pitch);

// MORSE --------------------------------------------------------------------------------

    /**
     * @brief Reproduce un buffer de audio PCM (mono, float32) generado en memoria, ej. morse.
     * @param audio Muestras PCM mono, float32.
     * @param sampleRate Frecuencia de muestreo del buffer.
     * @param volume Volumen del sonido (0 a 100)
     * @param pitch Tono del sonido (0.0f - )
     * @return Un identificador único para la instancia del sonido
     */

    unsigned long long playBuffer(std::vector<float> const& audio,
        unsigned int sampleRate,
        unsigned short volume = 100,
        bool loop = false,
        bool forceStop = false,
        unsigned short pitch = 1
    
    ); 
  
// Datos del módulo ---------------------------------------------------------------------

    /**
     * @brief Verifica si un sonido está en reproducción.
     * @param id Identificador del sonido.
     * @return true si el sonido está en reproducción, false en caso contrario.
     */
    bool isPlaying(unsigned long long id) const;

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

    /**
     * @brief Obtiene el ID del dispositivo.
     * @return Identificador del dispositivo.
     */
    ma_device_id getDeviceID() const;

    /**
     * @brief Callback llamado al finalizar la reproducción de un sonido.
     * @param userData Datos del usuario, típicamente el puntero a esta instancia.
     * @param sound Puntero al sonido que terminó de reproducirse.
     */
    static void endCallback(void* userData, ma_sound* sound);

    /**
     * @brief Limpia los sonidos que han terminado de reproducirse.
     */
    void cleanupFinished();


/************ Variables ****************************************************************/
    
// Aliases
    using SoundList = std::unordered_map<unsigned long long, std::unique_ptr<SoundInstance>>;

// Componentes de miniaudio
    ma_context*         context_;       ///< Contexto de mini audio.
    ma_device_info      device_info_;   ///< Información del dispositivo de audio.
    ma_engine           engine_;        ///< Motor de audio.
    
// Mapa de sonidos en reproducción
    SoundList           sounds_;        ///< Mapa de instancias de sonido.
    mutable std::mutex  sounds_mtx_;    ///< Mutex para sincronización de acceso a los recursos.

// Mapa de caché de sonido (revisar si hace lo mismo que lo de arriba)
    std::unordered_map<std::string, std::unique_ptr<ma_sound>>  cache_;     ///< Mapa de caché de sonidos.

    std::atomic<unsigned long long> idCounter_{1}; ///< Contador para los IDs de sonido.
    bool running_ = false;              ///< Indica si el motor de audio está en funcionamiento.

};

