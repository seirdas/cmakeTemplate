#include "sound/AudioPlaybackModule.hpp"
#include <mutex>
#include <queue>
#include <string>
#include <thread>


#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "system/SystemMgr.hpp"
    #include <memory>
    #include <unordered_map>
    #include <filesystem>           // Controla directorios, rutas, etc.


    // Implementación de miembros y métodos de la librería externa
    struct AudioPlaybackModule::Impl {

        // Aliases
        struct SoundInstance;
        using SoundList     = std::unordered_map<std::string, std::unique_ptr<SoundInstance>>;
        using CacheList     = std::unordered_map<std::string, std::unique_ptr<ma_sound>>;

        // Estructuras y enumerados
        /** @brief Elemento de audio en reproducción */
        struct SoundInstance
        {
            ma_sound    sound;              ///< La instancia del sonido en mini audio
            std::string name;               ///< Nombre del audio
            bool        loopMode   = false; ///< Indica reproducción en bucle del sonido
            bool        forceStop  = false; ///< Indica si el sonido debe pararse sin acabarlo completamente
        };

        // Componentes de miniaudio
        ma_context*     ctx;                ///< Contexto de miniaudio
        ma_device_info  device_info;        ///< Información del dispositivo de audio
        ma_engine       engine;             ///< Motor de audio

        // Listas de sonidos
        SoundList       playing_sounds;     ///< Mapa de instancias de sonido reproduciéndose (nombreWav -> SoundInstance)
        CacheList       sounds_cache;       ///< Mapa de caché de archivos de audio
        
        // Limpieza de sonidos terminados
        std::queue<std::unique_ptr<SoundInstance>> cleanup_queue;     ///< Cola de sonidos a desinicializar
        
        /**
        * @brief Constructor de Impl
        *  El constructor del Impl hace el cast de los punteros opacos
        * @param context (ma_context*) Contexto de audio de miniaudio
        * @param devInfo (ma_device_info*) Información del dispositivo
        */
        Impl(void* context, const void* devInfo);

        /**
        * @brief Marca como finalizado el sonido que acaba de reproducirse (finaliza)
        * @param userData Datos del usuario, típicamente el puntero a esta instancia.
        * @param sound Puntero al sonido que terminó de reproducirse.
        */
        static void endCallback(void* userData, ma_sound* sound);
    };


    // Implementación de métodos PIMPL ------------------------------------------------------

    AudioPlaybackModule::Impl::Impl(void* context, const void* devInfo) {
        ctx = static_cast<ma_context*>(context);
        device_info = *static_cast<const ma_device_info*>(devInfo);
    }

    void AudioPlaybackModule::Impl::endCallback(void* userData, ma_sound* sound) {
        auto* self = reinterpret_cast<AudioPlaybackModule*>(userData);

        // Busca el sonido en la lista de sonidos reproduciéndose para eliminarlo
        self->sendToCleanup(sound);
        self->cleanup_cv_.notify_one();
        //self->cleanup();
    }


    // General ------------------------------------------------------------------------------

    AudioPlaybackModule::AudioPlaybackModule(void* ctx, void* const device_info) :
        pimpl_(std::make_unique<Impl>(ctx, device_info)),
        initialized_(false)
    {

    }

    AudioPlaybackModule::~AudioPlaybackModule() {
        stop();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool AudioPlaybackModule::init() {
        if (initialized_)
            return true;

        // Inicializar el dispositivo de reproducción
        ma_engine_config config = ma_engine_config_init();
        config.pContext = pimpl_->ctx;
        config.pPlaybackDeviceID = &pimpl_->device_info.id; 
        if (ma_engine_init(&config, &pimpl_->engine) != MA_SUCCESS) {
            SYS_ERROR("AudioPlaybackModule","Engine init failed");
            return false;
        }            

        // Activar running para los hilos
        running_ = true;

        // Inicializar el hilo de limpieza diferida
        SYS_INFO("APM","Starting cleanup thread...");
        cleanup_thread_ = std::thread(&AudioPlaybackModule::TCleanup, this);

        initialized_ = true;
        return true;
    }

    void AudioPlaybackModule::stop() {
        if (!initialized_)
            return;

        running_ = false;

        // Apagar el hilo de limpieza primero
        if (cleanup_thread_.joinable()) {
            SYS_INFO("PlaybackModule","Closing cleanup thread...");
            cleanup_cv_.notify_all(); // Despertar al hilo para que finalice
            cleanup_thread_.join();
        }

        // Limpiar instancias de sonidos activos
        {
            std::lock_guard<std::mutex> lock(playing_sounds_mtx_);
            SYS_INFO("PlaybackModule","Unitializing active sounds...");
            for (auto& [id, snd] : pimpl_->playing_sounds)
                ma_sound_uninit(&snd->sound);
            pimpl_->playing_sounds.clear();
        }

        // Limpiar cache de audios
        {
            std::lock_guard<std::mutex> lock(sounds_cache_mtx_);
            SYS_INFO("PlaybackModule","Cleaning audio cache...");
            for (auto& [wavname, snd] : pimpl_->sounds_cache) 
                ma_sound_uninit(snd.get());
            pimpl_->sounds_cache.clear();
        }

        // Desinicializar el sistema
        SYS_INFO("PlaybackModule","Unitializing audio engine...");
        ma_engine_uninit(&pimpl_->engine);

        initialized_ = false;
        return;
    }


    // Acciones -----------------------------------------------------------------------------

    void AudioPlaybackModule::playAudio(
        std::string const&  filepath,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        if (!initialized_)
            return;

        // Guardar audio en caché si no estaba
        if (!pimpl_->sounds_cache.count(filepath))
            preloadAudioFile(filepath);
    
        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();
        ma_result res = ma_sound_init_copy(
            &pimpl_->engine,
            pimpl_->sounds_cache[filepath].get(),
            0,
            nullptr,
            &inst->sound);

        if (res != MA_SUCCESS) {
            SYS_WARN("AudioPlaybackModule","playAudio error.");
            return;
        }

        // Establecer parámetros de la reproducción
        ma_sound_set_volume(&inst->sound, static_cast<float>(volume)/100);  // (va de 0.0 a 1.0)
        ma_sound_set_pitch(&inst->sound, pitch);
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE : MA_FALSE);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        // Obtener el nombre del archivo para el mapa
        std::filesystem::path path = std::filesystem::absolute(filepath);
        std::string filename = path.stem().string();

        // Guardar la instancia de sonido en la lista de sonidos reproduciéndose
        std::unique_lock<std::mutex> soundsLock(playing_sounds_mtx_);

        // Guardar parámetros en la instancia de sonido guardado
        inst->loopMode  = loop;
        inst->forceStop = forceStop;
        inst->name      = filename;

        pimpl_->playing_sounds[filename] = std::move(inst);

        // Comenzar a reproducir (tomo directamente el sonido de la lista de playing_sounds)
        SYS_INFO("APM","'" + filename + "': init playing...");
        ma_sound_start(&pimpl_->playing_sounds[filename]->sound);
    }

    void AudioPlaybackModule::stopAudio(std::string const& audioName, bool force) {

        // Buscar el sonido reproduciéndose
        std::unique_lock<std::mutex> lock(playing_sounds_mtx_);
        auto it = pimpl_->playing_sounds.find(audioName);
        if (it == pimpl_->playing_sounds.end())
            return;
        
        // Desbloquear mutex para evitar bloqueos de las funciones de después (sendToCleanup)
        lock.unlock();

        // Obtener la SoundInstance
        std::unique_ptr<AudioPlaybackModule::Impl::SoundInstance>& audioInstance = it->second;

        // Si LoopMode activo, desactivar loop y dejar que acabe 
        if(audioInstance->loopMode)
            ma_sound_set_looping(&audioInstance->sound, MA_FALSE);
        else if(audioInstance->forceStop || force) {
            // Si forceStop de la instancia o param force activo, cortar inmediatamente
            ma_sound_stop(&audioInstance->sound);
            sendToCleanup(&audioInstance->sound);
            SYS_INFO("APM","'" + audioName + "': stop forced");
            cleanup_cv_.notify_one();
        }

        // si !forcestop, no hacer nada y dejar acabar el audio cuando termine
    }

    void AudioPlaybackModule::setVolume(std::string const& audioName, float volume) {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        float volume_normalized = static_cast<float>(volume)/100;

        auto it = pimpl_->playing_sounds.find(audioName);
        if (it != pimpl_->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "' volume changed: " + std::to_string((int)volume) + "/100");
            ma_sound_set_volume(&it->second->sound, volume_normalized);
        }
        else SYS_WARN("PlaybackModule", "Change volume fail: '" + audioName + "' not found");
    }

    void AudioPlaybackModule::setPitch(std::string const& audioName, float pitch) {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        auto it = pimpl_->playing_sounds.find(audioName);
        if (it != pimpl_->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "': pitch changed:" + std::to_string(pitch));
            ma_sound_set_pitch(&it->second->sound, pitch);
        }
        else SYS_WARN("PlaybackModule", "Change pitch fail: '" + audioName + "' not found");
    }


    // Datos del módulo ---------------------------------------------------------------------

    bool AudioPlaybackModule::isPlaying(std::string const& name) const {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        auto it = pimpl_->playing_sounds.find(name);
        if (it == pimpl_->playing_sounds.end())
            return false;

        return ma_sound_is_playing(&it->second->sound);
    }

    // #TODO CAMBIAR: de alguna manera, obtener un dispositivo libre que no esté reproduciendo (el siguiente, por ejemplo)
    bool AudioPlaybackModule::isBusy() const {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_); 

        // Devuelve si hay algún sonido en la lista de playing_sounds
        return !pimpl_->playing_sounds.empty();
    }

    std::string AudioPlaybackModule::deviceName() const {
        return pimpl_->device_info.name;
    }


    // Caché --------------------------------------------------------------------------------

    bool AudioPlaybackModule::preloadAudioFile(const std::string& filepath) {
        if (!initialized_)
            return false;

        std::unique_lock<std::mutex> lock(sounds_cache_mtx_);

        // Comprobar si ya está precargado
        if (pimpl_->sounds_cache.count(filepath))
            return true;

        // Desbloquear para la carga
        lock.unlock();

        // Inicializar el audio a partir del archivo
        auto snd = std::make_unique<ma_sound>();
        if (ma_sound_init_from_file(
                &pimpl_->engine,
                filepath.c_str(),
                MA_SOUND_FLAG_DECODE,
                nullptr,
                nullptr,
                snd.get()) != MA_SUCCESS)
        {
            return false;
        }

        // Bloquear de nuevo para el acceso a la caché
        lock.lock();

        // Guardar el audio generado en la caché
        pimpl_->sounds_cache[filepath] = std::move(snd);

        return true;
    }


    // Limpieza -----------------------------------------------------------------------------

    void AudioPlaybackModule::TCleanup() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        // Bucle de mientras haya sonidos pendientes de desinicializar
        while ( running_ || !pimpl_->cleanup_queue.empty()) {

            std::unique_lock<std::mutex> lock(cleanup_mtx_);
            // Esperar hasta que haya un sonido en cola o se ordene apagar
            cleanup_cv_.wait(lock, [this]() {
                return !pimpl_->cleanup_queue.empty() || !running_;
            });

            // Si se ordenó apagar y ya no quedan sonidos por limpiar, salir del bucle
            if (!running_ && pimpl_->cleanup_queue.empty())
                break;

            // Extraer el sonido de la cola
            if (!pimpl_->cleanup_queue.empty()) {
                instanceToClean = std::move(pimpl_->cleanup_queue.front());
                pimpl_->cleanup_queue.pop();
            }

            // Libera el lock de aquí en adelante
            lock.unlock();

            // Si tenemos la instancia extraída:
            if (instanceToClean) {
                // Desinicializar el recurso de miniaudio de forma segura en este hilo
                ma_sound_uninit(&instanceToClean->sound);

                // Al llegar al final de este bloque, instanceToClean se destruye automáticamente
                // liberando la memoria de SoundInstance sin tocar playing_sounds (ya no estaba ahí).
            }
        }
    }

    void AudioPlaybackModule::sendToCleanup(void* sound) {
        ma_sound* maSound = static_cast<ma_sound*>(sound);

        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);
        
        // Buscar la clave del sonido que ha terminado
        for (auto it = pimpl_->playing_sounds.begin(); it != pimpl_->playing_sounds.end(); ++it) {
            if (&it->second->sound == maSound) {

                // Notificar que el audio ha terminado
                SYS_INFO("PlaybackModule","'"+it->second->name+"' finished");
                
                // Extraer el nodo del mapa sin destruir la memoria del unique_ptr
                auto node = pimpl_->playing_sounds.extract(it);
                
                // Mover la propiedad de la instancia a la cola de limpieza
                {
                    std::lock_guard<std::mutex> cleanLock(cleanup_mtx_);
                    pimpl_->cleanup_queue.push(std::move(node.mapped()));
                }                

                // Avisar al hilo de limpieza que hay trabajo pendiente
                cleanup_cv_.notify_one();
                break;
            }
        }
    }

    void AudioPlaybackModule::cleanupSounds() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        std::lock_guard<std::mutex> lock(cleanup_mtx_);
        while (!pimpl_->cleanup_queue.empty()) {
            instanceToClean = std::move(pimpl_->cleanup_queue.front());
            pimpl_->cleanup_queue.pop();
            if (instanceToClean) 
                ma_sound_uninit(&instanceToClean->sound);
        }
    }


#endif
