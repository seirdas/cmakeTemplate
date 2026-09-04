#include "sound/PlayerAudio.hpp"
#include "sound/AudioPlaybackModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "system/SystemMgr.hpp"
    #include <miniaudio.h>
    #include <unordered_map>
    #include "sound/APM_Imp.hpp"         // PIMPL de AudioPlaybackModule


    // Implementación de miembros y métodos para PIMPL HIJA (Añade cosas al PIMPL del padre)
    struct PlayerAudioImpl : public AudioPlaybackModule::Impl {

        // Estructuras y enumerados
        /** @brief Elemento de caché de archivo de audio */
        struct AudioCacheInstance {
            ma_sound    sound;                              ///< Archivo de audio cacheado
            std::chrono::steady_clock::time_point time;     ///< Marca de tiempo en el que se cacheó el audio
        };

        // Aliases
        using CacheList         = std::unordered_map<std::string, std::unique_ptr<AudioCacheInstance>>;

        // Listas de sonidos
        CacheList           sounds_cache;           ///< Mapa de caché de archivos de audio
        mutable std::mutex  sounds_cache_mtx_;      ///< Mutex para la lista de caché de sonidos


        // Añadir constructor para llamar al de la base
        PlayerAudioImpl(void* ctx)
            : AudioPlaybackModule::Impl(ctx) {}
        
    };


    // General ------------------------------------------------------------------------------

    PlayerAudio::PlayerAudio(std::string const& moduleName, void* ctx) :
        AudioPlaybackModule(moduleName, std::make_unique<PlayerAudioImpl>(ctx)),
        keep_alive_seconds_(200)
    {
        // Añade cosas al Impl de la clase padre
    }

    
    // Inicialización -----------------------------------------------------------------------

    bool PlayerAudio::init(void* config) {
        // Iniciar el padre y comprobar que lo hace bien
        if (!AudioPlaybackModule::init(config))
            return false;

        // Iniciar el hilo del tiempo de vida de la caché de audios
        SYS_INFO("PlaybackModule","Starting audio cache cleanup on timeout thread...");
        cachereaper_thread_ = std::thread(&PlayerAudio::t_cache_reaper, this);

        return true;
    }

    bool PlayerAudio::close() {
        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        threads_running_ = false;

        // Despertar y unir hilo del Reaper de caché
        if (cachereaper_thread_.joinable()) {
            SYS_INFO("PlaybackModule","'" + name_ + "': Closing sounds reaper thread...");
            cachereaper_cv_.notify_all();
            cachereaper_thread_.join();
        }
        
        // Limpiar cache de audios
        {
            std::lock_guard<std::mutex> lock(pimpl_hija->sounds_cache_mtx_);
            SYS_INFO("PlaybackModule","Cleaning audio cache...");
            for (auto& [wavname, snd] : pimpl_hija->sounds_cache)
                ma_sound_uninit(&snd->sound);
            pimpl_hija->sounds_cache.clear();
        }

        // Ejecutar el close() del padre
        return AudioPlaybackModule::close();
    }


    // Ejecución ----------------------------------------------------------------------------

    void PlayerAudio::playAudio(
        std::string const&  filepath,
        const std::string&  deviceAlias,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        // Comprobar si el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("PlaybackModule", "playAudio called but module is not initialized");
            return;
        }

        // Resolver el dispositivo de playback por su alias
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlaybackModule", "playAudio: device alias '" + deviceAlias + "' not found/initialized");
            return;
        }

        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Clave compuesta
        const std::string cacheKey = make_cache_key(deviceAlias, filepath);

        // Asegurar precarga de audio en caché (devuelve true si ya estaba precargado)
        if (!preload_audio_on_cache(filepath, deviceAlias)) {
            SYS_WARN("AudioPlaybackModule", "Could not preload: " + filepath);
            return;
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Leer audio en caché 
        {
            // Proteger caché
            std::lock_guard<std::mutex> cacheLock(pimpl_hija->sounds_cache_mtx_);
            auto it = pimpl_hija->sounds_cache.find(cacheKey);
            if (it == pimpl_hija->sounds_cache.end()) 
            {
                SYS_WARN("PlaybackModule", "'" + name_ + "' playAudio: cannot found '" + filepath + "' on cache");
                return;
            }

            // Leer muestra precargada
            ma_result res = ma_sound_init_copy(
                &device->engine,
                &it->second->sound,
                0,
                nullptr,
                &inst->sound);

            if (res != MA_SUCCESS) {
                SYS_WARN("AudioPlaybackModule", "ma_sound_init_copy error");
                return;
            }
        }

        // Establecer parámetros de la reproducción
        float ma_volume = (static_cast<float>(volume) / 100.0f) * static_cast<float>(globalVol_) / 100.0f;
        ma_sound_set_volume(&inst->sound, ma_volume);  // (va de 0.0 a 1.0)
        ma_sound_set_pitch(&inst->sound, pitch);
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE : MA_FALSE);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, device);

        // Guardar parámetros en la instancia de sonido guardado
        inst->loopMode  = loop;
        inst->forceStop = forceStop;
        inst->name      = filepath;
        inst->volume    = volume;

        // Guardar en el mapa y reproducir
        {
            // Guardar la instancia de sonido en la lista de sonidos reproduciéndose
            std::lock_guard<std::mutex> soundsLock(device->playing_sounds_mtx);
            device->playing_sounds[cacheKey] = std::move(inst);

            // Comenzar a reproducir (tomo directamente el sonido de la lista de playing_sounds)
            SYS_INFO("PlaybackModule","'" + filepath + "': init playing on '" + deviceAlias + "'...");
            ma_sound_start(&device->playing_sounds[cacheKey]->sound);
        }
    }

    
    // Caché --------------------------------------------------------------------------------
    
    std::string PlayerAudio::make_cache_key(const std::string& deviceAlias, const std::string& filepath) {
        return deviceAlias + "|" + filepath;
    }

    bool PlayerAudio::preload_audio_on_cache(
        const std::string& filepath, 
        const std::string& deviceAlias)
    {
        // Comprobar si el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("PlaybackModule", "preload_audio_on_cache called but module is not initialized");
            return false;
        }

        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Obtener instancia de dispositivo
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlayerAudio", "preload: device alias '" + deviceAlias + "' not found");
            return false;
        }

        // Obtener clave compuesta de sonido en caché
        const std::string cacheKey = make_cache_key(deviceAlias, filepath);

        // Proteger el mapa de caché de sonidos
        std::unique_lock<std::mutex> lock(pimpl_hija->sounds_cache_mtx_);

        // Comprobar si ya está precargado
        if (pimpl_hija->sounds_cache.count(cacheKey))
            return true;

        // Desproteger mapa para la carga de miniaudio
        lock.unlock();

        // Crear una instancia de la estructura de la caché de audios
        std::unique_ptr<PlayerAudioImpl::AudioCacheInstance> inst = 
            std::make_unique<PlayerAudioImpl::AudioCacheInstance>();

        // Inicializar el audio de la instancia a partir del archivo
        if (ma_sound_init_from_file(
            &device->engine,
            filepath.c_str(),
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            &inst->sound) != MA_SUCCESS)
        {
            SYS_WARN("AudioPlaybackModule", "Failed to load audio file: " + filepath 
                + " on device '" + deviceAlias + "'");
            return false;
        }

        // Guardar la marca de tiempo
        std::unique_lock<std::mutex> reaperLock(cachereaper_mtx_);
        inst->time = std::chrono::steady_clock::now();
        reaperLock.unlock();

        // Bloquear de nuevo para el acceso a la caché
        lock.lock();

        // Guardar el audio generado en la caché
        pimpl_hija->sounds_cache[cacheKey] = std::move(inst);

        return true;
    }

    void PlayerAudio::t_cache_reaper() {

        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Tiempo de comprobación, cada tiempoVida/10 con mínimo de 1seg
        std::chrono::duration<long> poll_interval = std::max(std::chrono::seconds(1), keep_alive_seconds_ / 10);

        // Mutex del reaper (se desbloquea en el wait_for)
        std::unique_lock<std::mutex> lock(cachereaper_mtx_);

        while (threads_running_) {

            // Espera hasta: cierre, haya algo que vigilar
            cachereaper_cv_.wait_for(lock, poll_interval, [this] {
                return !threads_running_;
            });

            // Salir si no está activo el módulo (se está cerrando)
            if (!threads_running_) break;

            // Bloquear mutex de la lista de sonidos
            std::unique_lock<std::mutex> soundsLock(pimpl_hija->sounds_cache_mtx_);

            // Comprobar si hay algo que comprobar (xd)
            if (pimpl_hija->sounds_cache.empty()) continue;

            // Obtiene el tiempo actual para comparar
            auto now = std::chrono::steady_clock::now();

            // Borrar si se ha superado el tiempo y no se está usando (así para evitar segmentation-fault)
            for (auto it = pimpl_hija->sounds_cache.begin(); it != pimpl_hija->sounds_cache.end(); ) {
                if (now - it->second->time >= keep_alive_seconds_) {

                    // Comprobar si esa cacheKey sigue en reproducción EN ALGÚN device
                    bool isCurrentlyPlaying = false;
                    {
                        std::lock_guard<std::mutex> devicesLock(pimpl_->devices_mtx);
                        for (auto& dev : pimpl_->devices) {
                            std::lock_guard<std::mutex> pLock(dev->playing_sounds_mtx);
                            if (dev->playing_sounds.count(it->first)) {
                                isCurrentlyPlaying = true;
                                break;
                            }
                        }
                    }

                    if (!isCurrentlyPlaying) {
                        SYS_INFO("PlaybackModule", "Freeing cached sound: " + it->first);
                        ma_sound_uninit(&it->second->sound);
                        it = pimpl_hija->sounds_cache.erase(it);
                    } else {
                        it->second->time = now;
                        ++it;
                    }
                } else {
                    ++it;
                }
            }

            // Desbloquear el mutex de la lista de sonidos
            soundsLock.unlock();
        }
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    PlayerAudio::PlayerAudio(std::string const& moduleName, void* ctx, const void* device_info)
        : AudioPlaybackModule(moduleName, ctx, device_info), audioFolder_(""), keep_alive_seconds_(0) { }

    // Inicialización -----------------------------------------------------------------------
    bool PlayerAudio::init(void* config, std::string const& playbackName) { return false; }
    bool PlayerAudio::close()                                               { return false; }

    // Ejecución ----------------------------------------------------------------------------
    void PlayerAudio::playAudio(const std::string&, unsigned short, bool, bool, unsigned short) { }
    void PlayerAudio::playFromFolder(std::string const&, unsigned short, bool, bool, unsigned short) { }

    // Parámetros del módulo ----------------------------------------------------------------
    void PlayerAudio::setAudioFolder(std::string const& audioFolder)        { audioFolder_ = audioFolder; }

    // Privados / Caché ---------------------------------------------------------------------
    bool PlayerAudio::preload_audio_on_cache(const std::string&)            { return false; }
    void PlayerAudio::t_cache_reaper()                                      { }

#endif
