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
        PlayerAudioImpl(void* ctx, const void* device_info)
            : AudioPlaybackModule::Impl(ctx, device_info) {}
        
    };


    // General ------------------------------------------------------------------------------

    PlayerAudio::PlayerAudio(std::string const& moduleName, void* ctx, const void* device_info) :
        AudioPlaybackModule(moduleName, std::make_unique<PlayerAudioImpl>(ctx, device_info))
    {
        // Añade cosas al Impl de la clase padre
    }

    
    // Inicialización -----------------------------------------------------------------------

    bool PlayerAudio::init(
        void*               config, 
        std::string const&  playbackName)
    {
        // Iniciar el padre y comprobar que lo hace bien
        if (!AudioPlaybackModule::init(config, playbackName))
            return false;

        // Iniciar el hilo del tiempo de vida de la caché de audios
        SYS_INFO("PlaybackModule","Starting audio cache cleanup on timeout thread...");
        cachereaper_thread_ = std::thread(&PlayerAudio::t_cache_reaper, this);

        return true;
    }

    bool PlayerAudio::close() {
        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

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
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Comprobar si el contexto está inicializado
        if (!initialized_)
            return;

        // Asegurar precarga de audio en caché (devuelve true si ya estaba precargado)
        if (!preload_audio_on_cache(filepath)) {
            SYS_WARN("AudioPlaybackModule", "Could not preload: " + filepath);
            return;
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Bloquear caché para leer la muestra precargada de forma segura
        {
            std::lock_guard<std::mutex> cacheLock(pimpl_hija->sounds_cache_mtx_);
            auto it = pimpl_hija->sounds_cache.find(filepath);
            if (it == pimpl_hija->sounds_cache.end()) return;

            ma_result res = ma_sound_init_copy(
                &pimpl_->engine,
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
        float pan = (selectedChannel_ == 1) ? -1.0f : (selectedChannel_ == 2) ? 1.0f : 0.0f;
        ma_sound_set_pan(&inst->sound, pan);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        // Guardar parámetros en la instancia de sonido guardado
        inst->loopMode  = loop;
        inst->forceStop = forceStop;
        inst->name      = filepath;
        inst->volume    = volume;

        // Guardar en el mapa y reproducir
        {
            // Guardar la instancia de sonido en la lista de sonidos reproduciéndose
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[filepath] = std::move(inst);

            // Comenzar a reproducir (tomo directamente el sonido de la lista de playing_sounds)
            SYS_INFO("PlaybackModule","'" + filepath + "': init playing...");
            ma_sound_start(&pimpl_->playing_sounds[filepath]->sound);
        }
    }

    void PlayerAudio::playFromFolder(
        std::string const&  filename,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        if (audioFolder_.empty()) {
            SYS_WARN("AudioPlaybackModule", "playFromFolder: audioFolder not defined");
            return;
        }

        // Construye la ruta completa a partir de la carpeta configurada y el nombre del archivo
        std::filesystem::path fullPath = std::filesystem::path(audioFolder_) / filename;

        // Reproducir
        playAudio(fullPath.string(), volume, loop, forceStop, pitch);
    }


    // Parámetros del módulo ----------------------------------------------------------------

    void PlayerAudio::setAudioFolder(std::string const& audioFolder) {
        audioFolder_ = audioFolder;
    }

    
    // Caché --------------------------------------------------------------------------------
    
    bool PlayerAudio::preload_audio_on_cache(const std::string& filepath) {

        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Comprobar si el contexto está inicializado
        if (!initialized_)
            return false;

        // Proteger el mapa de caché de sonidos
        std::unique_lock<std::mutex> lock(pimpl_hija->sounds_cache_mtx_);

        // Comprobar si ya está precargado
        if (pimpl_hija->sounds_cache.count(filepath))
            return true;

        // Desproteger mapa para la carga de miniaudio
        lock.unlock();

        // Crear una instancia de la estructura de la caché de audios (es como un AudioCacheInstance* inst)
        std::unique_ptr<PlayerAudioImpl::AudioCacheInstance> inst = 
            std::make_unique<PlayerAudioImpl::AudioCacheInstance>();

        // Inicializar el audio de la instancia a partir del archivo
        if (ma_sound_init_from_file(
            &pimpl_->engine,
            filepath.c_str(),
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            &inst->sound) != MA_SUCCESS)
        {
            SYS_WARN("AudioPlaybackModule", "Failed to load audio file: " + filepath);
            return false;
        }

        // Guardar la marca de tiempo
        std::unique_lock<std::mutex> reaperLock(cachereaper_mtx_);
        inst->time = std::chrono::steady_clock::now();
        reaperLock.unlock();

        // Bloquear de nuevo para el acceso a la caché
        lock.lock();

        // Guardar el audio generado en la caché
        pimpl_hija->sounds_cache[filepath] = std::move(inst);

        return true;
    }

    void PlayerAudio::t_cache_reaper() {

        // Obtener PIMPL de esta clase hija
        PlayerAudioImpl* pimpl_hija = static_cast<PlayerAudioImpl*>(pimpl_.get());

        // Tiempo de comprobación, cada tiempoVida/10 con mínimo de 1seg
        std::chrono::duration<long> poll_interval = std::max(std::chrono::seconds(1), keep_alive_seconds_ / 10);

        // Mutex del reaper (se desbloquea en el wait_for)
        std::unique_lock<std::mutex> lock(cachereaper_mtx_);

        while (running_) {

            // Espera hasta: cierre, haya algo que vigilar
            cachereaper_cv_.wait_for(lock, poll_interval, [this] {
                return !running_;
            });

            // Salir si no está activo el módulo (se está cerrando)
            if (!running_) break;

            /* #TODO */

            // Bloquear mutex de la lista de sonidos
            std::unique_lock<std::mutex> soundsLock(pimpl_hija->sounds_cache_mtx_);

            // Comprobar si hay algo que comprobar (xd)
            if (pimpl_hija->sounds_cache.empty()) continue;

            // Compara el tiempo de los modelos con el actual
            auto now = std::chrono::steady_clock::now();

            // Borrar si se ha superado el tiempo y no se está usando (así para evitar segmentation-fault)
            bool isCurrentlyPlaying = false;
            for (auto it = pimpl_hija->sounds_cache.begin(); it != pimpl_hija->sounds_cache.end(); ) {
                if (now - it->second->time >= keep_alive_seconds_) {

                    // No liberar de la caché si hay algún sonido sonando
                    isCurrentlyPlaying = false;
                    {
                        std::unique_lock<std::mutex> pLock(playing_sounds_mtx_);
                        // Comprobar si la ruta coincide con alguno de los activos
                        if (pimpl_->playing_sounds.count(it->first))
                            isCurrentlyPlaying = true;
                    }

                    if (!isCurrentlyPlaying) {
                        SYS_INFO("PlaybackModule", "Freeing cached sound: " + it->first);
                        ma_sound_uninit(&it->second->sound);
                        it = pimpl_hija->sounds_cache.erase(it);
                    } else {
                        // Renovar expiración para reintentar en el siguiente ciclo
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
PlayerAudio::PlayerAudio(std::string const&, void*, const void*) :
    AudioPlaybackModule("", nullptr, nullptr)
{}

// Ejecución ----------------------------------------------------------------------------
void PlayerAudio::playFromFolder(
    std::string const&,
    unsigned short,
    bool,
    bool,
    unsigned short 
) { return; }

// Parámetros del módulo ----------------------------------------------------------------
void PlayerAudio::setAudioFolder(std::string const&)     { return; }

#endif
