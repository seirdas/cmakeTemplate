#include "sound/PlayerTTS.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "miniaudio.h"
    #include "system/SystemMgr.hpp"
    #include "sound/APM_Imp.hpp"


// General ------------------------------------------------------------------------------

    PlayerTTS::PlayerTTS(std::string const& moduleName, void* ctx) :
        AudioPlaybackModule(moduleName, ctx),
        onTextToAudio_cb_(nullptr)
    {

    }

    PlayerTTS::~PlayerTTS() {
        close();
    }


// Inicialización -----------------------------------------------------------------------

    bool PlayerTTS::init(void* config) {
        if (!AudioPlaybackModule::init(config))
            return false;

        // Arrancar el hilo consumidor de textos
        SYS_INFO("PlayerTTS","Starting text consumer thread...");
        data_consumer_thread_ = std::thread(&PlayerTTS::t_data_consumer, this);

        return true;
    }

    bool PlayerTTS::close() {
        if (!initialized_)
            return true;

        SYS_INFO("PlayerTTS","Stopping text consumer thread...");

        // Marcar como no corriendo para que el hilo consumidor salga del bucle
        threads_running_ = false;
        cola_textos_cv_.notify_all();

        if (data_consumer_thread_.joinable())
            data_consumer_thread_.join();

        // Cierra el motor de audio y el resto de hilos de la clase padre
        return AudioPlaybackModule::close();
    }


// Ejecución ----------------------------------------------------------------------------

    bool PlayerTTS::playTTS(
        std::string const&  text, 
        std::string const&  modelName, 
        std::string const&  deviceAlias,
        std::string const&  audioName,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {

        // Comprobaciones previas
        if (!initialized_)
            return false;
        if (text.empty()) {
            SYS_WARN("PlayerTTS","Cannot play: Text empty");
            return false;
        }
        if (modelName.empty()) {
            SYS_WARN("PlayerTTS","Cannot play: ModelName empty");
            return false;
        }
        if (!onTextToAudio_cb_) {
            SYS_WARN("PlayerTTS","Cannot generate audio: TTS callback function don't defined");
            return false;
        }
        
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("PlaybackModule", "playAudio: device alias '" + deviceAlias + "' not found/initialized");
            return false;
        }

        // Generar audio (muestras + sample rate real del modelo usado)
        AudioData audio = onTextToAudio_cb_(modelName, text);
        if (audio.samples.empty()) {
            SYS_WARN("PlayerTTS", "Empty audio generated from " + modelName);
            texto_en_proceso_.clear();
            return false;
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Describir el audio generado (formato, canales, tamaño, puntero a los datos)
        ma_audio_buffer_config config = ma_audio_buffer_config_init(
            ma_format_f32,
            1,
            audio.samples.size(),
            audio.samples.data(),
            nullptr
        );
        config.sampleRate = audio.sample_rate;

        // Copiar los datos generados al buffer de la instancia
        if (ma_audio_buffer_init_copy(&config, &inst->buffer) != MA_SUCCESS) {
            SYS_WARN("PlayerTTS", "reproducir_elemento: fallo al crear el buffer de audio");
            return false;
        }
        inst->hasBuffer = true;

        // Envolver el buffer como un sonido reproducible por el motor
        if (ma_sound_init_from_data_source(&device->engine, &inst->buffer, 0, nullptr, &inst->sound) != MA_SUCCESS) {
            ma_audio_buffer_uninit(&inst->buffer);
            SYS_WARN("PlayerTTS", "reproducir_elemento: fallo al inicializar el sonido");
            texto_en_proceso_.clear();
            return false;
        }

        // Establecer parámetros de la reproducción
        float ma_volume = (static_cast<float>(volume) / 100.0f) * static_cast<float>(globalVol_) / 100.0f;
        ma_sound_set_volume(&inst->sound, ma_volume);  // (va de 0.0 a 1.0)
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE : MA_FALSE);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, device);

        // Guardar parámetros en la instancia de sonido
        inst->loopMode  = loop;
        inst->forceStop = forceStop;
        inst->name      = audioName;
        inst->volume    = volume;
        inst->pitch     = pitch;

        // Guardar en el mapa del dispositivo y reproducir
        {
            std::lock_guard<std::mutex> soundsLock(device->playing_sounds_mtx);
            device->playing_sounds[audioName] = std::move(inst);

            SYS_INFO("PlayerTTS","'" + audioName + "': init playing TTS on '" + deviceAlias + "'...");
            ma_sound_start(&device->playing_sounds[audioName]->sound);
        }

        // Limpiar el texto que se está procesando
        texto_en_proceso_.clear();

        // Notificar que ha terminado de reproducir (iComm)
        /* #TODO */

        return true;
    }


// Inyección de función texto a audio ---------------------------------------------------

    void PlayerTTS::setCallback_onTextToAudio(TTSFunction fn) {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = std::move(fn); 
    }

    void PlayerTTS::clearCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = nullptr;
    }

    bool PlayerTTS::hasCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        return static_cast<bool>(onTextToAudio_cb_);
    }


// Hilos --------------------------------------------------------------------------------

    // #TODO Nunca se llama, he probado primero a reproducir normal. hay que implementar
    void PlayerTTS::t_data_consumer() {

        // Elemento a reproducir de la cola
        queueElement element;

        while(threads_running_) {

            // Salir si el programa se está cerrando (antes)
            if (!threads_running_) break;

            // Forzar espera hasta que haya algo en la cola o se cierre este player
            std::unique_lock<std::mutex> lock(cola_textos_mtx_);
            cola_textos_cv_.wait(lock, [this] {
                return !threads_running_ || !cola_textos_.empty();
            });

            // Salir si el programa se está cerrando (después)
            if(!threads_running_) break;

            // Obtener tarea de la cola
            element = cola_textos_.front();
            cola_textos_.pop();

            // Libera el lock de aquí en adelante
            lock.unlock();

            // Reproducir elemento
            reproducir_elemento(element);
        }

        SYS_INFO("PlayerTTS::t_data_consumer", "Thread stopped.");

    }


// Procesado de elementos de la cola ----------------------------------------------------

    bool PlayerTTS::reproducir_elemento(queueElement element) {
        // #TODO
        return false;
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
PlayerTTS::PlayerTTS(std::string const& moduleName, void* ctx, const void* device_info)
    : AudioPlaybackModule(moduleName, ctx, device_info), playbackName_(moduleName), texto_en_proceso_("") { }
PlayerTTS::~PlayerTTS()                                 { }

// Inicialización -----------------------------------------------------------------------
bool PlayerTTS::init(void* config, std::string const& playbackName) { return false; }
bool PlayerTTS::close()                                 { return false; }

// Ejecución ----------------------------------------------------------------------------
bool PlayerTTS::playTTS(std::string const&, std::string const&, std::string const&, unsigned short, bool, bool) { return false; }

// Inyección de función texto a audio ---------------------------------------------------
void PlayerTTS::setCallback_onTextToAudio(TTSFunction)  { }
void PlayerTTS::clearCallback_onTextToAudio()           { }
bool PlayerTTS::hasCallback_onTextToAudio()             { return false; }

// Privados -----------------------------------------------------------------------------
void PlayerTTS::t_data_consumer()                       { }
bool PlayerTTS::reproducir_elemento(queueElement)       { return false; }

#endif
