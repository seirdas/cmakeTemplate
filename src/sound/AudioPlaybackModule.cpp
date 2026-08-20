#include "sound/AudioPlaybackModule.hpp"
#include <mutex>


#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "system/SystemMgr.hpp"
    #include "files/JsonMgr.hpp"
    #include <filesystem>               // Controla directorios, rutas, etc.
    
    #include "sound/APM_Imp.hpp"         // PIMPL de AudioPlaybackModule

    // General ------------------------------------------------------------------------------

    AudioPlaybackModule::AudioPlaybackModule(std::string const& moduleName, void* ctx, const void* device_info) :
        pimpl_(std::make_unique<Impl>(ctx, device_info)),
        initialized_(false),
        running_(false),
        name_(moduleName),
        globalVol_(100),
        selectedChannel_(0),
        keep_alive_seconds_(10),
        active_fadeouts_threads_(0)
    {

    }

    AudioPlaybackModule::~AudioPlaybackModule() {
        close();
    }


    // Inicialización -----------------------------------------------------------------------

    bool AudioPlaybackModule::init(
        void*               config, 
        std::string const&  moduleName) 
    {
        if (initialized_)
            return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);


        // Ponerle nombre a este módulo (sobreescribe el de la config)
        if (!moduleName.empty()) name_ = moduleName;

        // Inicializar el dispositivo de reproducción
        ma_engine_config deviceConfig = ma_engine_config_init();

        // rellenar los parámetros de la configuración de miniaudio
        deviceConfig.pContext = pimpl_->ctx;
        deviceConfig.pPlaybackDeviceID = &pimpl_->device_info.id;

        std::string deviceName = pimpl_->device_info.name;

        // Inicializar
        if (ma_engine_init(&deviceConfig, &pimpl_->engine) != MA_SUCCESS) {
            SYS_ERROR("AudioPlaybackModule", moduleName + ": ma_engine_init failed");
            return false;
        }

        // Activar running para los hilos
        running_ = true;

        // Inicializar el hilo de limpieza diferida
        SYS_INFO("PlaybackModule","Starting audio cleanup thread...");
        cleanup_thread_ = std::thread(&AudioPlaybackModule::t_cleanup, this);

        // Iniciar el hilo del tiempo de vida de la caché de audios
        SYS_INFO("PlaybackModule","Starting audio cache cleanup on timeout thread...");
        cachereaper_thread_ = std::thread(&AudioPlaybackModule::t_cache_reaper, this);

        // Llega hasta aquí si se ha inicializado bien
        initialized_ = true;
        return initialized_; //<- true
    }

    bool AudioPlaybackModule::isInitialized() const {
        return initialized_;
    }

    void AudioPlaybackModule::loadConfig(void* config) {
        if (!config)
             return;

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        jsonMgr.get_or_set(cfg, "name", name_);
        
        /* Esto ya llega en la inicialización, en devInfo del constructor */
        //jsonMgr.get_or_set(cfg, "device", device_);
    }

    bool AudioPlaybackModule::close() {
        if (!initialized_)
            return true;

        SYS_INFO("PlaybackModule", "'" + name_ + "': Stopping PlaybackModule...");

        running_ = false;

        // Espera segura de hilos pitchout
        if (active_fadeouts_threads_ > 0) {
            SYS_INFO("PlaybackModule", "'" + name_ + "': Waiting for active pitchout threads to finish...");
            std::unique_lock<std::mutex> lock(playing_sounds_mtx_);
            
            // Espera bloqueante hasta que el contador de hilos llegue a 0
            pitch_threads_cv_.wait(lock, [this]() { 
                return active_fadeouts_threads_ == 0; 
            });
        }

        // Apagar el hilo de limpieza primero
        if (cleanup_thread_.joinable()) {
            SYS_INFO("PlaybackModule", "'" + name_ + "':Closing cleanup thread...");
            cleanup_cv_.notify_all(); // Despertar al hilo para que finalice
            cleanup_thread_.join();
        }

        // Despertar y unir hilo del Reaper de caché
        if (cachereaper_thread_.joinable()) {
            SYS_INFO("PlaybackModule","'" + name_ + "': Closing sounds reaper thread...");
            cachereaper_cv_.notify_all();
            cachereaper_thread_.join();
        }

        // Limpiar instancias de sonidos activos
        {
            std::lock_guard<std::mutex> lock(playing_sounds_mtx_);
            SYS_INFO("PlaybackModule","'" + name_ + "': Unitializing active sounds...");
            for (auto& [id, snd] : pimpl_->playing_sounds) {
                ma_sound_uninit(&snd->sound);
                if (snd->isBuffer)
                    ma_audio_buffer_uninit(&snd->buffer);
            }
            pimpl_->playing_sounds.clear();
        }
        cleanup_sounds();    // Asegurar limpieza

        // Limpiar cache de audios
        {
            std::lock_guard<std::mutex> lock(sounds_cache_mtx_);
            SYS_INFO("PlaybackModule","Cleaning audio cache...");
            for (auto& [wavname, snd] : pimpl_->sounds_cache)
                ma_sound_uninit(&snd->sound);
            pimpl_->sounds_cache.clear();
        }

        // Desinicializar el sistema
        SYS_INFO("PlaybackModule","'" + name_ + "': Unitializing audio engine...");
        ma_engine_uninit(&pimpl_->engine);

        initialized_ = false;
        return true;
    }

    bool AudioPlaybackModule::reload() {

        SYS_INFO("PlaybackModule", "Reloading module...");

        // Parar todo si está inicializado
        if (initialized_)
            close();

        // Inicializa tomando los parámetros nuevos para la config (samplerate, channels, etc.)
        return init();
    }


    // Ejecución ----------------------------------------------------------------------------

    void AudioPlaybackModule::playAudio(
        std::string const&  filepath,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        if (!initialized_)
            return;

        // Asegurar precarga de audio en caché (devuelve true si ya estaba precargado)
        if (!preload_audio_file(filepath)) {
            SYS_WARN("AudioPlaybackModule", "Could not preload: " + filepath);
            return;
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Bloquear caché para leer la muestra precargada de forma segura
        {
            std::lock_guard<std::mutex> cacheLock(sounds_cache_mtx_);
            auto it = pimpl_->sounds_cache.find(filepath);
            if (it == pimpl_->sounds_cache.end()) return;

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

    void AudioPlaybackModule::stop(
        std::string const& audioName, 
        bool force,
        unsigned int fadeOutMs,
        unsigned int pitchOutMs )
    {
        // Info
        std::string stopType = (force) ? "forced" : "soft";
        SYS_INFO("PlaybackModule", "Commanded " + stopType + " stop to '" + audioName + "'");

        // Buscar el sonido reproduciéndose
        std::unique_lock<std::mutex> lock(playing_sounds_mtx_);
        auto it = pimpl_->playing_sounds.find(audioName);
        if (it == pimpl_->playing_sounds.end()) {
            SYS_WARN("PlaybackModule","Error stopping audio: '" + audioName + "' not playing");
            return;
        }

        // Obtener la SoundInstance
        std::unique_ptr<AudioPlaybackModule::Impl::SoundInstance>& inst = it->second;

        // 1. Gestión de parada "inmediata" -----------------------------------
        if (fadeOutMs == 0 && pitchOutMs == 0) {

            // Si LoopMode activo, desactivar loop y dejar que acabe (lo gestiona el callback)
            if(inst->loopMode) {
                ma_sound_set_looping(&inst->sound, MA_FALSE);
                inst->loopMode = false;
            }
    
            // Si forceStop de la instancia o param force activo, cortar inmediatamente
            if(inst->forceStop || force) {

                // Anulación de callback para evitar que miniaudio limpie duplicado al hacer stop
                ma_sound_set_end_callback(&inst->sound, nullptr, nullptr);

                // Liberar el lock para evitar bloqueo en send_to_cleanup
                lock.unlock();

                // Manda el sonido a limpiar
                stop_and_send_to_cleanup(&inst->sound);
                SYS_INFO("PlaybackModule","'" + audioName + "': stop forced");
            }

            // si !forcestop, no hacer nada y dejar acabar el audio cuando termine

            // !! SALE AQUÍ Y NO EJECUTA LÓGICA DE PARADA PROGRESIVA
            return;
        }

        // 2. Gestión de parada progresiva ------------------------------------

        // Gestión de parada solo con fadeOut (mantendría loop si tuviera)
        if (fadeOutMs > 0) {
            SYS_INFO("PlaybackModule", "'" + audioName + "': fadeOut stop initiated");
            start_fadeout_thread(audioName, &inst->sound, fadeOutMs);
        }

        // Gestión de parada con pitchOut (hilo manual)
        if (pitchOutMs > 0) {
            SYS_INFO("PlaybackModule", "'" + audioName + "': pitchOut stop initiated");
            start_pitchout_thread(
                audioName, 
                &inst->sound, 
                pitchOutMs, 
                ma_sound_get_pitch(&inst->sound),   // toma el pìtch aquí bajo mutex
                (fadeOutMs == 0) ? true : false     // Indica si debe limpiar el audio o no
            );
            
        }
    }

    void AudioPlaybackModule::setVolume(std::string const& audioName, unsigned short volume) {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        float volume_normalized = static_cast<float>(volume)/100;

        auto it = pimpl_->playing_sounds.find(audioName);
        if (it != pimpl_->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "' volume changed: " + std::to_string((int)volume) + "/100");
            ma_sound_set_volume(&it->second->sound, volume_normalized);
        }
        else SYS_WARN("PlaybackModule", "Change volume fail: '" + audioName + "' not found");
    }

    void AudioPlaybackModule::setModuleVolume(unsigned short volume) {

        // Limitar el volumen global entre 0 y 100
        globalVol_ = (volume < 100) ? volume : 100;

        // Proteger la lista de sonidos activos con el mutex del módulo
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        // Actualizar los sonidos en reproducción utilizando su volumen base
        float finalVolume = 0;
        float globalFactor = static_cast<float>(globalVol_) / 100.0f;
        for (auto& [id, inst] : pimpl_->playing_sounds) {
            finalVolume = inst->volume * globalFactor;
            ma_sound_set_volume(&inst->sound, finalVolume);
        }
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

    void AudioPlaybackModule::setSelectedChannel(unsigned short channel){
        selectedChannel_ = channel; 
    }


    // Parámetros del módulo ----------------------------------------------------------------

    std::string AudioPlaybackModule::getDeviceName() const {
        return pimpl_->device_info.name;
    }

    std::string AudioPlaybackModule::getModuleName() const {
        return name_;
    }

    unsigned short AudioPlaybackModule::getModuleVolume() const {
        return globalVol_;
    }

    bool AudioPlaybackModule::isPlaying(std::string const& audioName) const {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        // Si no hay nombre, devuelve si la lista de sonidos en reproducción está vacía
        if(audioName.empty()) 
            return !pimpl_->playing_sounds.empty();

        // si hay nombre, busca si ese nombre está en la lista de sonidos en reproducción
        auto it = pimpl_->playing_sounds.find(audioName);
        if (it == pimpl_->playing_sounds.end())
            return false;

        return ma_sound_is_playing(&it->second->sound);
    }
    

    // Caché --------------------------------------------------------------------------------

    bool AudioPlaybackModule::preload_audio_file(const std::string& filepath) {
        if (!initialized_)
            return false;

        std::unique_lock<std::mutex> lock(sounds_cache_mtx_);

        // Comprobar si ya está precargado
        if (pimpl_->sounds_cache.count(filepath))
            return true;

        // Desbloquear para la carga
        lock.unlock();

        // Crear una instancia de la estructura de la caché de audios
        auto inst = std::make_unique<Impl::AudioCacheInstance>();

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
        pimpl_->sounds_cache[filepath] = std::move(inst);

        return true;
    }


    // Limpieza -----------------------------------------------------------------------------

    void AudioPlaybackModule::t_cleanup() {

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

                // Si venía de un buffer generado (ej. morse), liberar también ese buffer
                if (instanceToClean->isBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);

                // Al llegar al final de este bloque, instanceToClean se destruye automáticamente
                // liberando la memoria de SoundInstance sin tocar playing_sounds (ya no estaba ahí).
            }
        }
    }

    void AudioPlaybackModule::stop_and_send_to_cleanup(void* sound) {
        ma_sound* maSound = static_cast<ma_sound*>(sound);

        // Parar el sonido
        ma_sound_stop(maSound);

        // Proteger la lista PlayingSounds
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        // Buscar la clave del sonido que ha terminado
        for (auto it = pimpl_->playing_sounds.begin(); it != pimpl_->playing_sounds.end(); ++it) {
            if (&it->second->sound == maSound) {

                // Notificar que el audio ha terminado
                SYS_INFO("PlaybackModule", "'" + it->second->name + "': audio finished");

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

    void AudioPlaybackModule::cleanup_sounds() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        std::lock_guard<std::mutex> lock(cleanup_mtx_);
        while (!pimpl_->cleanup_queue.empty()) {
            instanceToClean = std::move(pimpl_->cleanup_queue.front());
            pimpl_->cleanup_queue.pop();
            if (instanceToClean) {
                ma_sound_uninit(&instanceToClean->sound);
                if (instanceToClean->isBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);
            }
        }
    }


    // Limpieza de caché de audios ----------------------------------------------------------

    void AudioPlaybackModule::t_cache_reaper() {

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
            std::unique_lock<std::mutex> soundsLock(sounds_cache_mtx_);

            // Comprobar si hay algo que comprobar (xd)
            if (pimpl_->sounds_cache.empty()) continue;

            // Compara el tiempo de los modelos con el actual
            auto now = std::chrono::steady_clock::now();

            // Borrar si se ha superado el tiempo y no se está usando (así para evitar segmentation-fault)
            bool isCurrentlyPlaying = false;
            for (auto it = pimpl_->sounds_cache.begin(); it != pimpl_->sounds_cache.end(); ) {
                if (now - it->second->time >= keep_alive_seconds_) {

                    // No liberar de la caché si hay algún sonido sonando
                    isCurrentlyPlaying = false;
                    {
                        std::unique_lock<std::mutex> pLock(playing_sounds_mtx_);
                        // Comprobar si la ruta coincide con alguno de los activos
                        std::filesystem::path p(it->first);
                        std::string stem = p.stem().string();
                        if (pimpl_->playing_sounds.count(stem)) {
                            isCurrentlyPlaying = true;
                        }
                    }

                    if (!isCurrentlyPlaying) {
                        SYS_INFO("PlaybackModule", "Freeing cached sound: " + it->first);
                        ma_sound_uninit(&it->second->sound);
                        it = pimpl_->sounds_cache.erase(it);
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


    // Threads de paradas progresivas -------------------------------------------------------------------

    void AudioPlaybackModule::start_fadeout_thread(
        std::string const&  audioName,
        void*               soundPtr,
        unsigned int        totalTransitionMs)
    {

        // Obtener el sonido en su tipo ma_audio
        ma_sound* sound = static_cast<ma_sound*>(soundPtr);

        // Activar el fadeout de miniaudio
        ma_sound_set_fade_in_milliseconds(
            sound, ma_sound_get_volume(sound), 0.0f, totalTransitionMs);

        // Incrementar el contador de hilos activos antes de lanzar
        active_fadeouts_threads_++;

        // Iniciar un hilo para gestionar la limpieza del sonido cuando acaba el fadeout
        std::thread([this, audioName, sound, totalTransitionMs]() {
            struct ThreadGuard {
                AudioPlaybackModule* module;
                ~ThreadGuard() {
                    module->active_fadeouts_threads_--;
                    module->pitch_threads_cv_.notify_all();
                }
            } guard{this};

            // Esperar el tiempo exacto del fadeout
            std::this_thread::sleep_for(std::chrono::milliseconds(totalTransitionMs));

            if (!running_) return;

            std::unique_lock<std::mutex> asyncLock(playing_sounds_mtx_);

            // Verificar que el sonido no haya sido destruido durante el sleep
            auto it = pimpl_->playing_sounds.find(audioName);
            if (it != pimpl_->playing_sounds.end() && &it->second->sound == sound) {
                
                // Quitar loop y anular callback para evitar invocaciones duplicadas
                ma_sound_set_looping(sound, MA_FALSE);
                it->second->loopMode = false;
                ma_sound_set_end_callback(sound, nullptr, nullptr);

                // Unlocking previo a send_to_cleanup
                asyncLock.unlock();

                // Parar y enviar a limpiar
                stop_and_send_to_cleanup(sound);
            }
        }).detach();
    }

    void AudioPlaybackModule::start_pitchout_thread(
        std::string const& audioName,
        void* soundPtr,
        unsigned int totalTransitionMs,
        float startPitch,
        bool cleanup)
    {
        // Incrementar el contador de hilos activos antes de lanzar
        active_fadeouts_threads_++;

        std::thread([this, audioName, soundPtr, totalTransitionMs, startPitch, cleanup]() {

            // Estructura lambda para decrementar el contador de hilos al salir siempre (RAII)
            struct ThreadGuard {
                AudioPlaybackModule* module;
                ~ThreadGuard() {
                    module->active_fadeouts_threads_--;
                    module->pitch_threads_cv_.notify_all(); // Avisar a close() si está esperando
                }
            } guard{this};
            
            // Tiempo transcurrido entre bajadas de pitch
            const unsigned int stepMs = 20;

            // Tiempo transcurrido en el while
            unsigned int elapsedTime = 0;

            // Obtener el sonido en su tipo ma_audio
            ma_sound* sound = static_cast<ma_sound*>(soundPtr);

            // Bajar progresivamente el pitch en el tiempo establecido
            while (elapsedTime < totalTransitionMs) {

                // Abortar si se está cerrando (antes de esperar)
                if (!running_)
                    return;

                // Esperar el tiempo entre bajadas de pitch
                std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
                elapsedTime += stepMs;

                // Abortar si se está cerrando (después de esperar)
                if (!running_)
                    return;

                // Proteger la lista de playingSounds
                std::lock_guard<std::mutex> asyncLock(playing_sounds_mtx_);

                // Comprobar si el sonido aún existe (pudo haber sido limpiado por endCallback u otro evento)
                auto itAsync = pimpl_->playing_sounds.find(audioName);
                if (itAsync == pimpl_->playing_sounds.end() || &itAsync->second->sound != sound) {
                    return;
                }

                // Aplicar bajada de pitch
                float progress = static_cast<float>(elapsedTime) / static_cast<float>(totalTransitionMs);
                float currentPitch = startPitch * (1.0f - progress);
                ma_sound_set_pitch(sound, std::max(0.01f, currentPitch));
            }

            SYS_INFO("PlaybackModule", "'" + audioName + "': pitchout stop complete");


            // Comprobar si este hilo se encarga de limpiar el audio
            if (!cleanup)
                return;

            // Proteger la lista de playingSounds
            std::unique_lock<std::mutex> asyncLock(playing_sounds_mtx_);

            // Comprobar si el sonido aún existe
            auto itAsync = pimpl_->playing_sounds.find(audioName);
            if (itAsync != pimpl_->playing_sounds.end() && &itAsync->second->sound == sound) {

                // Apagar el modo loop (opcional)
                ma_sound_set_looping(sound, MA_FALSE);
                itAsync->second->loopMode = false;

                // Anulación de callback para evitar que miniaudio limpie duplicado al hacer stop
                ma_sound_set_end_callback(sound, nullptr, nullptr);

                // Unlocking previo a send_to_cleanup
                asyncLock.unlock();

                // Parar y enviar a limpiar
                stop_and_send_to_cleanup(sound);
            }
        }).detach();
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct AudioPlaybackModule::Impl {};

// General ------------------------------------------------------------------------------
AudioPlaybackModule::AudioPlaybackModule(void* ctx, const void* device_info) {}
AudioPlaybackModule::~AudioPlaybackModule()  {}

// Inicialización y ejecución -----------------------------------------------------------
bool AudioPlaybackModule::init(void*, std::string const&)    { return false; }
bool AudioPlaybackModule::isInitialized() const              { return false; }
void AudioPlaybackModule::loadConfig(void*)                  { return; }
bool AudioPlaybackModule::close()                            { return false; }
bool AudioPlaybackModule::reload()                           { return false; }

// Acciones -----------------------------------------------------------------------------
void AudioPlaybackModule::playAudio(const std::string&, unsigned short, bool, bool, unsigned short) { return; }
void AudioPlaybackModule::stop(std::string const&, bool, unsigned int, unsigned int)           { return; }
void AudioPlaybackModule::setVolume(std::string const&, unsigned short) { return; }
void AudioPlaybackModule::setModuleVolume(unsigned short)               { return; }
void AudioPlaybackModule::setPitch(std::string const&, float)           { return; }

// Parámetros del módulo ----------------------------------------------------------------
std::string AudioPlaybackModule::getDeviceName() const           { return "";    }
std::string AudioPlaybackModule::getModuleName() const           { return "";    }
bool AudioPlaybackModule::isPlaying(std::string const&) const    { return false; }
bool AudioPlaybackModule::isBusy() const                         { return false; }

// Caché --------------------------------------------------------------------------------
bool AudioPlaybackModule::preload_audio_file(const std::string&)   { return false; }

// Limpieza de sonidos ------------------------------------------------------------------
void AudioPlaybackModule::t_cleanup()                           { return; }
void AudioPlaybackModule::stop_and_send_to_cleanup(void*)       { return; }
void AudioPlaybackModule::cleanup_sounds()                      { return; }

// Limpieza de caché de audios ----------------------------------------------------------
void AudioPlaybackModule::t_cache_reaper()                  { return; }

// Thread de pitchOut -------------------------------------------------------------------
    void start_fadeout_thread(
        std::string const&,
        void*,
        unsigned int
    ) { return; }
    void start_pitchout_thread(
        std::string const&,
        void*,
        unsigned int,
        float,
        bool
    ) { return; }



#endif
