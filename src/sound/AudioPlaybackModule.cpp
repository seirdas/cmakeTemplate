#include "sound/AudioPlaybackModule.hpp"
#include <mutex>


#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "system/SystemMgr.hpp"
    #include "files/JsonMgr.hpp"
    
    #include "sound/APM_Imp.hpp"         // PIMPL de AudioPlaybackModule


    // General ------------------------------------------------------------------------------

    AudioPlaybackModule::AudioPlaybackModule(
        std::string const&  moduleName, 
        void*               ctx
    ) :
        pimpl_(std::make_unique<Impl>(ctx)),
        initialized_(false),
        threads_running_(false),
        name_(moduleName),
        globalVol_(100),
        selectedChannel_(0),
        active_fadeouts_threads_(0)
    {

    }

    AudioPlaybackModule::~AudioPlaybackModule() {
        close();
    }


    // Inicialización -----------------------------------------------------------------------

    bool AudioPlaybackModule::init(void* config) {
        if (initialized_)
            return false;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

        // Activar running para los hilos
        threads_running_ = true;

        // Inicializar el hilo de limpieza diferida
        SYS_INFO("PlaybackModule","Starting audio cleanup thread...");
        cleanup_thread_ = std::thread(&AudioPlaybackModule::t_cleanup, this);
        
        // Llega hasta aquí si se ha inicializado bien
        initialized_ = true;
        return true;
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
        
        // Si no está inicializado, no hacer nada
        if (!initialized_)
            return false;

        // Marcar el módulo como no inicializado y detener hilos
        threads_running_ = false;

        SYS_INFO("PlaybackModule", "'" + name_ + "': Stopping PlaybackModule...");

        // Espera de hilos de pitchout/fadeout
        if (active_fadeouts_threads_ > 0) {
            SYS_INFO("PlaybackModule", "'" + name_ + "': Waiting for active pitchout threads to finish...");

            // Esperar a que no haya ningun sonido "apagándose"
            std::unique_lock<std::mutex> lock(fadeout_threads_mtx_);
            fadeout_threads_cv_.wait(lock, [this]() { 
                return active_fadeouts_threads_ == 0; 
            });
        }

        // Apagar el hilo de limpieza
        if (cleanup_thread_.joinable()) {
            SYS_INFO("PlaybackModule", "'" + name_ + "': Closing cleanup thread...");
            cleanup_cv_.notify_all(); // Despertar al hilo para que finalice su bucle
            cleanup_thread_.join();
        }

        // Detener y limpiar los sonidos de todos los dispositivos registrados
        SYS_INFO("PlaybackModule", "'" + name_ + "': Uninitializing active sounds across all devices...");
        std::lock_guard<std::mutex> devicesLock(pimpl_->devices_mtx);
        for (auto& DevInst : pimpl_->devices) {
            if (!DevInst) continue;

            // Proteger el mapa de sonidos en reproducción
            std::lock_guard<std::mutex> lock(DevInst->playing_sounds_mtx);
            
            // Detener todas las reproducciones de este dispositivo
            for (auto& [id, snd] : DevInst->playing_sounds)
                if (snd) {
                    ma_sound_stop(&snd->sound);
                    if (snd->hasBuffer) 
                        ma_audio_buffer_uninit(&snd->buffer);
                }

            // Limpiar el mapa
            DevInst->playing_sounds.clear();
        }
        // Limpiar la cola diferida de sonidos pendientes por destruir
        cleanup_sounds();

        // Desinicializar motores y dispositivos iniciados en el Impl
        /* (Lista protegida antes) */
        for (auto& DevInst : pimpl_->devices) {
            if (!DevInst || !DevInst->initialized) 
                continue;

            ma_engine_uninit(&DevInst->engine);
            ma_device_uninit(&DevInst->device);
            DevInst->initialized = false;
        }

        // Limpieza de todos los dispositivos
        pimpl_->devices.clear();

        // Marcar como no inicializado y salir
        initialized_ = false;
        SYS_INFO("PlaybackModule", "'" + name_ + "': Closed successfully.");
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


    // Dispositivos del módulo --------------------------------------------------------------

    bool AudioPlaybackModule::addPlaybackDevice(
        std::string const&  deviceName, 
        unsigned int        channelSelected,
        std::string const&  deviceAlias) 
    {
        // Comprobar si el contexto está inicializado
        if (!pimpl_->ctx) {
            SYS_WARN("AudioPlayback", "Cannot add new device: audio context not initialized");
            return false;
        }

        // Comprobar resolutor de dispositivos
        if (!onDeviceResolve_) {
            SYS_WARN("AudioPlayback","Cannot add new device: Can't resolve device info.");
            return false;
        }

        // Resolver alias efectivo: el explícito, o uno autogenerado a partir de nombre+canal
        std::string effectiveAlias = deviceAlias.empty() 
            ? (deviceName + "#" + std::to_string(channelSelected)) 
            : deviceAlias;

        // Proteger la lista de dispositivos para toda la operación de alta
        std::lock_guard<std::mutex> devicesLock(pimpl_->devices_mtx);

        // Comprobar que el alias no está ya en uso (evita colisiones)
        for (auto& dev : pimpl_->devices) {
            if (dev->alias == effectiveAlias) {
                SYS_WARN("AudioPlayback", "'" + name_ + "': alias '" + effectiveAlias + "' already in use");
                return false;
            }
        }

        // Obtiene la información del dispositivo (ma_device_info)
        std::string realDeviceName = deviceName;
        const ma_device_info* selectedDeviceInfo = 
            static_cast<const ma_device_info*>(onDeviceResolve_(realDeviceName));
        if (!selectedDeviceInfo) {
            SYS_WARN("SoundMgr", "Failed to find device: '" + deviceName + "'");
            return false;
        }

        // Nº de canales REALES del dispositivo
        unsigned int channels = selectedDeviceInfo->nativeDataFormats[0].channels;
        if (channelSelected > channels) {
            SYS_WARN("PlaybackModule","'" + name_ + "': selected_channel " + std::to_string(channelSelected)
                + " not available (Channels:" + std::to_string(channels) + ")");
            return false;
        }

        // Instanciar el nuevo dispositivo
        std::unique_ptr<Impl::DeviceInstance> instance = std::make_unique<Impl::DeviceInstance>();
        instance->alias           = effectiveAlias;
        instance->info            = *selectedDeviceInfo;
        instance->selectedChannel = channelSelected;
        instance->channels        = channels;
        instance->owner           = this;

        // Configurar ma_device
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.pDeviceID = &selectedDeviceInfo->id;
        deviceConfig.playback.format    = ma_format_f32;

        // Establecer el enrutado de canal ANTES de ma_device_init
        ma_channel channelMap[1];
        if (channelSelected == 0) {
            // Todos los canales disponibles seleccionados
            deviceConfig.playback.channels    = channels;
            deviceConfig.playback.pChannelMap = nullptr;
        } else {
            // Submix mono automático de miniaudio al canal AUX correspondiente
            deviceConfig.playback.channels    = 1;
            channelMap[0] = static_cast<ma_channel>(MA_CHANNEL_AUX_0 + (channelSelected - 1));
            deviceConfig.playback.pChannelMap = channelMap;
        }

        // Inicializar device
        if (ma_device_init(pimpl_->ctx, &deviceConfig, &instance->device) != MA_SUCCESS) {
            SYS_WARN("AudioPlayback", "'" + name_ + "': failed to init device " + instance->info.name);
            return false;
        }

        // Configurar ma_engine vinculado a este ma_device
        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.pDevice  = &instance->device;
        engineConfig.channels = (channelSelected == 0) ? 0 : 1; // 0=todos/1=mono

        // Inicializar engine
        if (ma_engine_init(&engineConfig, &instance->engine) != MA_SUCCESS) {
            SYS_WARN("AudioPlayback", "'" + name_ + "': failed to init engine for " + instance->info.name);
            ma_device_uninit(&instance->device);
            return false;
        }

        // Marcar como inicializado y agregar al vector de dispositivos
        instance->initialized = true;
        pimpl_->devices.push_back(std::move(instance));

        SYS_INFO("AudioPlayback", "'" + name_ + "': added device '" + effectiveAlias + "' (" 
                + selectedDeviceInfo->name + ", channel " + std::to_string(channelSelected) + ")");
        return true;
    }

    bool AudioPlaybackModule::removePlaybackDevice(std::string const& deviceAlias) {

        // Proteger la lista de dispositivos para toda la operación de baja
        std::lock_guard<std::mutex> devicesLock(pimpl_->devices_mtx);

        // Localizar el device por alias
        auto it = std::find_if(pimpl_->devices.begin(), pimpl_->devices.end(),
            [&](std::unique_ptr<Impl::DeviceInstance> const& dev) {
                return dev->alias == deviceAlias;
            });

        if (it == pimpl_->devices.end()) {
            SYS_WARN("AudioPlayback", "'" + name_ + "': removePlaybackDevice: alias '" + deviceAlias + "' not found");
            return false;
        }

        Impl::DeviceInstance* device = it->get();

        SYS_INFO("AudioPlayback", "'" + name_ + "': removing device '" + deviceAlias + "'...");

        // Detener y desinicializar de forma segura todos los sonidos activos de ESTE device.
        // No pasan por la cola de limpieza diferida (t_cleanup): al eliminar el device
        // no tiene sentido esperar al hilo de limpieza, se libera aquí mismo de forma síncrona.
        {
            std::lock_guard<std::mutex> soundsLock(device->playing_sounds_mtx);
            for (auto& [name, snd] : device->playing_sounds) {
                ma_sound_uninit(&snd->sound);
                if (snd->hasBuffer)
                    ma_audio_buffer_uninit(&snd->buffer);
            }
            device->playing_sounds.clear();
        }

        // Eliminar del vector: el destructor de DeviceInstance libera engine/device
        // automáticamente (initialized == true en este punto)
        pimpl_->devices.erase(it);

        SYS_INFO("AudioPlayback", "'" + name_ + "': device '" + deviceAlias + "' removed");
        return true;
    }

    void AudioPlaybackModule::setCallback_onDeviceResolve(std::function<const void*(std::string&)> cb) {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        onDeviceResolve_ = std::move(cb); 
    }

    void AudioPlaybackModule::clearCallback_onDeviceResolve() {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        onDeviceResolve_ = nullptr;
    }

    bool AudioPlaybackModule::hasCallback_onDeviceResolve() const {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        return static_cast<bool>(onDeviceResolve_);
    }


    // Ejecución ----------------------------------------------------------------------------

    // REVISAR, SI ALIAS = "", APAGAR EN TODOS LOS DISPOSITIVOS
    void AudioPlaybackModule::stop(
        std::string const&  audioName, 
        std::string const&  deviceAlias, 
        bool                force,
        unsigned int        fadeOutMs,
        unsigned int        pitchOutMs )
    {
        // Resolver el dispositivo por alias
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlaybackModule", "stop: device alias '" + deviceAlias + "' not found/initialized");
            return;
        }

        // Info
        std::string stopType = (force) ? "forced" : "soft";
        SYS_INFO("PlaybackModule", "Commanded " + stopType + " stop to '" + audioName + "'");

        // Buscar el sonido reproduciéndose en el dispositivo seleccionado
        std::unique_lock<std::mutex> lock(device->playing_sounds_mtx);
        auto it = device->playing_sounds.find(audioName);
        if (it == device->playing_sounds.end()) {
            SYS_WARN("PlaybackModule","Error stopping audio: '" + audioName + "' not playing on '" + deviceAlias + "'");
            return;
        }

        // Obtener la SoundInstance
        std::unique_ptr<AudioPlaybackModule::Impl::SoundInstance>& inst = it->second;

        // 1. Gestión de parada "inmediata" -----------------------------------
        if (fadeOutMs == 0 && pitchOutMs == 0) {

            // Si LoopMode activo, desactivar loop y dejar que acabe (lo gestiona el callback)
            if (inst->loopMode) {
                ma_sound_set_looping(&inst->sound, MA_FALSE);
                inst->loopMode = false;
            }
    
            // Si forceStop de la instancia o param force activo, cortar inmediatamente
            if (inst->forceStop || force) {

                // Anulación de callback para evitar que miniaudio limpie duplicado al hacer stop
                ma_sound_set_end_callback(&inst->sound, nullptr, device);

                // Liberar el lock para evitar bloqueo en send_to_cleanup
                lock.unlock();

                // Manda el sonido a limpiar
                stop_and_send_to_cleanup(&inst->sound, device);
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
            start_fadeout_thread(audioName, device, &inst->sound, fadeOutMs);
        }

        // Gestión de parada con pitchOut (hilo manual)
        if (pitchOutMs > 0) {
            SYS_INFO("PlaybackModule", "'" + audioName + "': pitchOut stop initiated");
            start_pitchout_thread(
                audioName, 
                device,
                &inst->sound, 
                pitchOutMs, 
                ma_sound_get_pitch(&inst->sound),   // toma el pìtch aquí bajo mutex
                (fadeOutMs == 0) ? true : false     // Indica si debe limpiar el audio o no
            );
            
        }
    }

    void AudioPlaybackModule::setVolume(
        std::string const&  audioName, 
        unsigned short      volume, 
        std::string const&  deviceAlias)
    {
        // Localizar instancia de dispositivo por alias
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlaybackModule", "setVolume: device alias '" + deviceAlias + "' not found/initialized");
            return;
        }

        // Proteger lista de sonidos en reproducción
        std::lock_guard<std::mutex> lock(device->playing_sounds_mtx);

        // Normalizar volumen al rango de miniaudio (max=1.0f)
        float volume_normalized = static_cast<float>(volume) / 100;

        // Establecer el volumen del audio en reproducción
        auto it = device->playing_sounds.find(audioName);
        if (it != device->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "' volume changed: " + std::to_string((int)volume) + "/100 on '" + deviceAlias + "'");
            ma_sound_set_volume(&it->second->sound, volume_normalized);
        }
        else SYS_WARN("PlaybackModule", "Change volume fail: '" + audioName + "' not found on '" + deviceAlias + "'");
    }

    void AudioPlaybackModule::setModuleVolume(unsigned short volume) {

        // Limitar el volumen global entre 0 y 100
        globalVol_ = (volume < 100) ? volume : (unsigned short)100;

        // Normalizar a rango de miniaudio (max = 1.0f)
        float globalFactor = static_cast<float>(globalVol_) / 100.0f;

        // Iterar por cada dispositivo registrado
        for (auto& device : pimpl_->devices) {
            if (!device) continue;

            // Proteger la lista de sonidos activos de este dispositivo específico
            std::lock_guard<std::mutex> soundsLock(device->playing_sounds_mtx);

            // Recorrer audios y modificar volumen
            for (auto& [id, inst] : device->playing_sounds) {
                if (inst) {
                    float finalVolume = (static_cast<float>(inst->volume) / 100.0f) * globalFactor;
                    ma_sound_set_volume(&inst->sound, finalVolume);
                }
            }
        }
    }

    void AudioPlaybackModule::setPitch(
        std::string const&  audioName, 
        float               pitch, 
        std::string const&  deviceAlias) 
    {
        // Localizar instancia de dispositivo por alias
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlaybackModule", "setVolume: device alias '" + deviceAlias + "' not found/initialized");
            return;
        }

        // Proteger lista de sonidos en reproducción
        std::lock_guard<std::mutex> lock(device->playing_sounds_mtx);

        // Establecer el pitch del audio en reproducción
        auto it = device->playing_sounds.find(audioName);
        if (it != device->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "': pitch changed:" + std::to_string(pitch));
            ma_sound_set_pitch(&it->second->sound, pitch);
        }
        else SYS_WARN("PlaybackModule", "Change pitch fail: '" + audioName + "' not found");
    }


    // Parámetros del módulo ----------------------------------------------------------------

    std::string AudioPlaybackModule::getModuleName() const {
        return name_;
    }

    unsigned short AudioPlaybackModule::getModuleVolume() const {
        return globalVol_;
    }


    // Constructor para clases derivadas ----------------------------------------------------

    AudioPlaybackModule::AudioPlaybackModule(
        std::string const&      moduleName, 
        std::unique_ptr<Impl>   customImpl
    ) :
        pimpl_(std::move(customImpl)),
        initialized_(false),
        threads_running_(false),
        name_(moduleName),
        globalVol_(100),
        selectedChannel_(0),
        active_fadeouts_threads_(0)
    {

    }


    // Limpieza de sonidos ------------------------------------------------------------------

    void AudioPlaybackModule::t_cleanup() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        // Bucle de mientras haya sonidos pendientes de desinicializar
        while ( threads_running_ || !pimpl_->cleanup_queue.empty()) {

            std::unique_lock<std::mutex> lock(cleanup_mtx_);
            // Esperar hasta que haya un sonido en cola o se ordene apagar
            cleanup_cv_.wait(lock, [this]() {
                return !pimpl_->cleanup_queue.empty() || !threads_running_;
            });

            // Si se ordenó apagar y ya no quedan sonidos por limpiar, salir del bucle
            if (!threads_running_ && pimpl_->cleanup_queue.empty())
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
                if (instanceToClean->hasBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);

                // Al llegar al final de este bloque, instanceToClean se destruye automáticamente
                // liberando la memoria de SoundInstance sin tocar playing_sounds (ya no estaba ahí).
            }
        }
    }

    void AudioPlaybackModule::stop_and_send_to_cleanup(void* sound, void* deviceAlias) {
        // Obtener el sonido en su tipo ma_audio
        ma_sound* maSound = static_cast<ma_sound*>(sound);

        // Obtener el dispositivo en su tipo DeviceInstance
        Impl::DeviceInstance* devInst = static_cast<Impl::DeviceInstance*>(deviceAlias);

        // Parar el sonido
        ma_sound_stop(maSound);

        // Proteger la lista PlayingSounds
        std::lock_guard<std::mutex> lock(devInst->playing_sounds_mtx);

        // Buscar la clave del sonido que ha terminado
        for (auto it = devInst->playing_sounds.begin(); it != devInst->playing_sounds.end(); ++it) {
            if (&it->second->sound == maSound) {

                // Notificar que el audio ha terminado
                SYS_INFO("PlaybackModule", "'" + it->second->name + "': audio finished");

                // Extraer el nodo del mapa sin destruir la memoria del unique_ptr
                auto node = devInst->playing_sounds.extract(it);

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
                if (instanceToClean->hasBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);
            }
        }
    }


    // Threads de paradas progresivas -------------------------------------------------------------------

    void AudioPlaybackModule::start_fadeout_thread(
        std::string const&  audioName,
        void*               deviceAlias,
        void*               soundPtr,
        unsigned int        totalTransitionMs)
    {
        // Obtener el sonido en su tipo ma_audio
        ma_sound* sound = static_cast<ma_sound*>(soundPtr);

        // Obtener el dispositivo en su tipo DeviceInstance
        Impl::DeviceInstance* devInst = static_cast<Impl::DeviceInstance*>(deviceAlias);

        // Activar el fadeout de miniaudio
        ma_sound_set_fade_in_milliseconds(
            sound, ma_sound_get_volume(sound), 0.0f, totalTransitionMs);

        // Incrementar el contador de hilos activos antes de lanzar
        active_fadeouts_threads_++;

        // Iniciar un hilo para gestionar la limpieza del sonido cuando acaba el fadeout
        std::thread([this, audioName, devInst, sound, totalTransitionMs]() {
            struct ThreadGuard {
                AudioPlaybackModule* module;
                ~ThreadGuard() {
                    module->active_fadeouts_threads_--;
                    module->fadeout_threads_cv_.notify_all();
                }
            } guard{this};

            // Esperar el tiempo exacto del fadeout
            std::this_thread::sleep_for(std::chrono::milliseconds(totalTransitionMs));

            // Comprobar si los hilos deben seguir corriendo
            if (!threads_running_) return;

            // Proteger lista de sonidos en reproducción
            std::unique_lock<std::mutex> asyncLock(devInst->playing_sounds_mtx);

            // Verificar que el sonido no haya sido destruido durante el sleep
            auto it = devInst->playing_sounds.find(audioName);
            if (it != devInst->playing_sounds.end() && &it->second->sound == sound) {
                
                // Quitar loop y anular callback para evitar invocaciones duplicadas
                ma_sound_set_looping(sound, MA_FALSE);
                it->second->loopMode = false;
                ma_sound_set_end_callback(sound, nullptr, devInst);

                // Unlocking previo a send_to_cleanup
                asyncLock.unlock();

                // Parar y enviar a limpiar
                stop_and_send_to_cleanup(sound, devInst);
            }
        }).detach();
    }

    void AudioPlaybackModule::start_pitchout_thread(
        std::string const&  audioName,
        void*               deviceAlias,
        void*               soundPtr,
        unsigned int        totalTransitionMs,
        float               startPitch,
        bool                cleanup)
    {
        // Incrementar el contador de hilos activos antes de lanzar
        active_fadeouts_threads_++;

        // Obtener el dispositivo en su tipo DeviceInstance
        Impl::DeviceInstance* devInst = static_cast<Impl::DeviceInstance*>(deviceAlias);

        // Iniciar un hilo para gestionar la limpieza del sonido cuando acaba el fadeout
        std::thread([this, audioName, devInst, soundPtr, totalTransitionMs, startPitch, cleanup]() {

            // Estructura lambda para decrementar el contador de hilos al salir siempre (RAII)
            struct ThreadGuard {
                AudioPlaybackModule* module;
                ~ThreadGuard() {
                    module->active_fadeouts_threads_--;
                    module->fadeout_threads_cv_.notify_all(); // Avisar a close() si está esperando
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
                if (!threads_running_)
                    return;

                // Esperar el tiempo entre bajadas de pitch
                std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
                elapsedTime += stepMs;

                // Abortar si se está cerrando (después de esperar)
                if (!threads_running_)
                    return;

                // Proteger la lista de playingSounds
                std::lock_guard<std::mutex> asyncLock(devInst->playing_sounds_mtx);

                // Comprobar si el sonido aún existe (pudo haber sido limpiado por endCallback u otro evento)
                auto itAsync = devInst->playing_sounds.find(audioName);
                if (itAsync == devInst->playing_sounds.end() || &itAsync->second->sound != sound) {
                    return;
                }

                // Aplicar bajada de pitch
                float progress = static_cast<float>(elapsedTime) / static_cast<float>(totalTransitionMs);
                float currentPitch = startPitch * (1.0f - progress);
                ma_sound_set_pitch(sound, std::max(0.01f, currentPitch));
            }

            // Info
            SYS_INFO("PlaybackModule", "'" + audioName + "': pitchout stop complete");

            // Comprobar si este hilo se encarga de limpiar el audio
            if (!cleanup)
                return;

            // Proteger la lista de playingSounds
            std::unique_lock<std::mutex> asyncLock(devInst->playing_sounds_mtx);

            // Comprobar si el sonido aún existe
            auto itAsync = devInst->playing_sounds.find(audioName);
            if (itAsync != devInst->playing_sounds.end() && &itAsync->second->sound == sound) {

                // Apagar el modo loop (opcional)
                ma_sound_set_looping(sound, MA_FALSE);
                itAsync->second->loopMode = false;

                // Anulación de callback para evitar que miniaudio limpie duplicado al hacer stop
                ma_sound_set_end_callback(sound, nullptr, devInst);

                // Unlocking previo a send_to_cleanup
                asyncLock.unlock();

                // Parar y enviar a limpiar
                stop_and_send_to_cleanup(sound, devInst);
            }
        }).detach();
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    struct AudioPlaybackModule::Impl {};

    // General ------------------------------------------------------------------------------
    AudioPlaybackModule::AudioPlaybackModule(std::string const& moduleName, void*, const void*)
        : pimpl_(std::make_unique<Impl>()), 
        initialized_(false), running_(false), 
        name_(moduleName), 
        globalVol_(100), 
        selectedChannel_(0), 
        active_fadeouts_threads_(0)                         { }
    AudioPlaybackModule::AudioPlaybackModule(std::string const& moduleName, std::unique_ptr<Impl> customImpl)
        : pimpl_(std::move(customImpl)), 
        initialized_(false), 
        running_(false), 
        name_(moduleName), 
        globalVol_(100), 
        selectedChannel_(0), 
        active_fadeouts_threads_(0)                         { }
    AudioPlaybackModule::~AudioPlaybackModule()             { }

    // Inicialización -----------------------------------------------------------------------
    bool AudioPlaybackModule::init(void*, std::string const&) { return false; }
    bool AudioPlaybackModule::isInitialized() const         { return false; }
    void AudioPlaybackModule::loadConfig(void*)             { }
    bool AudioPlaybackModule::close()                       { return false; }
    bool AudioPlaybackModule::reload()                      { return false; }

    // Ejecución ----------------------------------------------------------------------------
    void AudioPlaybackModule::stop(std::string const&, bool, unsigned int, unsigned int) { }
    void AudioPlaybackModule::setVolume(std::string const&, unsigned short) { }
    void AudioPlaybackModule::setModuleVolume(unsigned short)               { }
    void AudioPlaybackModule::setPitch(std::string const&, float)           { }
    void AudioPlaybackModule::setSelectedChannel(unsigned short)            { }

    // Parámetros del módulo ----------------------------------------------------------------
    std::string AudioPlaybackModule::getDeviceName() const  { return ""; }
    std::string AudioPlaybackModule::getModuleName() const  { return ""; }
    unsigned short AudioPlaybackModule::getModuleVolume() const { return 0; }
    bool AudioPlaybackModule::isPlaying(std::string const&) const { return false; }

    // Limpieza y Hilos ---------------------------------------------------------------------
    void AudioPlaybackModule::t_cleanup()                       { }
    void AudioPlaybackModule::stop_and_send_to_cleanup(void*)   { }
    void AudioPlaybackModule::cleanup_sounds()                  { }
    void AudioPlaybackModule::start_fadeout_thread(std::string const&, void*, unsigned int) { }
    void AudioPlaybackModule::start_pitchout_thread(std::string const&, void*, unsigned int, float, bool) { }

#endif
