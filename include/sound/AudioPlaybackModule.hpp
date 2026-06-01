#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <miniaudio.h>

/**
 * @enum LoopMode
 * @brief Enum para definir los modos de repetición de sonido.
 */
enum class LoopMode
{
    NONE,   ///< No se repite el sonido.
    LOOP,   ///< Se repite el sonido en bucle.
    NONSTOP ///< Se reproduce infinitamente hasta que se detiene manualmente.
};

using SoundID = uint64_t; ///< Tipo de dato para almacenar la identificación del sonido.

struct SoundInstance
{
    ma_sound            sound;                      ///< La instancia del sonido en mini audio.
    LoopMode            loopMode = LoopMode::NONE;  ///< Modo de repetición del sonido.
    std::atomic<bool>   finished{false};            ///< Indica si el sonido ha terminado de reproducirse.
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
     * @param volume Volumen del sonido (0.0f a 1.0f).
     * @param pitch Tono del sonido.
     * @param loop Modo de repetición.
     * @return Un identificador único para la instancia del sonido.
     */
    SoundID play(const std::string& filepath,
                 float volume = 1.0f,
                 float pitch = 1.0f,
                 LoopMode loop = LoopMode::NONE);

    /**
     * @brief Detiene la reproducción de un sonido con el ID especificado.
     * @param id Identificador del sonido a detener.
     */
    void stopSound(SoundID id);

    /**
     * @brief Establece el volumen de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param volume Volumen del sonido (0.0f a 1.0f).
     */
    void setVolume(SoundID id, float volume);
    
    /**
     * @brief Establece el tono de un sonido en ejecución.
     * @param id Identificador del sonido.
     * @param pitch Tono del sonido.
     */
    void setPitch(SoundID id, float pitch);


// Datos del módulo ---------------------------------------------------------------------

    /**
     * @brief Verifica si un sonido está en reproducción.
     * @param id Identificador del sonido.
     * @return true si el sonido está en reproducción, false en caso contrario.
     */
    bool isPlaying(SoundID id) const;

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
    const ma_device_id getDeviceID() const;

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
    
    ma_context*         context_;       ///< Contexto de mini audio.
    ma_device_info      device_info_;   ///< Información del dispositivo de audio.
    ma_engine           engine_;        ///< Motor de audio.
    mutable std::mutex  mutex_;         ///< Mutex para sincronización de acceso a los recursos.

    std::unordered_map<SoundID, std::unique_ptr<SoundInstance>> sounds_;    ///< Mapa de instancias de sonido.
    std::unordered_map<std::string, std::unique_ptr<ma_sound>>  cache_;     ///< Mapa de caché de sonidos.

    std::atomic<SoundID> idCounter_{1}; ///< Contador para los IDs de sonido.
    bool running_ = false;              ///< Indica si el motor de audio está en funcionamiento.
};
