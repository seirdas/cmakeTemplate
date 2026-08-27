#include "sound/PlayerTTS.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "miniaudio.h"
    #include "system/SystemMgr.hpp"
    #include "sound/APM_Imp.hpp"


// General ------------------------------------------------------------------------------

    PlayerTTS::PlayerTTS(std::string const& moduleName, void* ctx, const void* device_info) :
        AudioPlaybackModule(moduleName, ctx, device_info),
        onTextToAudio_cb_(nullptr)
    {

    }

    PlayerTTS::~PlayerTTS() {
        close();
    }


// Inicialización -----------------------------------------------------------------------

    bool PlayerTTS::init(void* config, std::string const& playbackName) {

        // Guardar si ya estaba inicializado antes de llamar al padre (evita relanzar el hilo)
        bool wasInitialized = initialized_;

        if (!AudioPlaybackModule::init(config, playbackName))
            return false;

        // Arrancar el hilo consumidor de textos solo la primera vez
        if (!wasInitialized) {
            SYS_INFO("PlayerTTS","Starting text consumer thread...");
            data_consumer_thread_ = std::thread(&PlayerTTS::t_data_consumer, this);
        }

        return true;
    }

    bool PlayerTTS::close() {
        if (!initialized_)
            return true;

        SYS_INFO("PlayerTTS","Stopping text consumer thread...");

        // Marcar como no corriendo para que el hilo consumidor salga del bucle
        running_ = false;
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
        std::string const&  audioName,
        unsigned short      volume,
        bool                loop,
        bool                forceStop)
    {

        // Comprobaciones previas
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

        std::lock_guard<std::mutex> lock(cola_textos_mtx_);

        // Encolamos los datos para que el hilo los procese
        queueElement element;
        element.modelName   = modelName;
        element.text        = text;
        element.audioName   = (audioName.empty()) ? text : audioName;
        element.volume      = volume;
        element.loop        = loop;
        element.forceStop   = forceStop;
        cola_textos_.push(element);

        // Despertamos al hilo de procesamiento si estaba dormido
        cola_textos_cv_.notify_one();

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

    void PlayerTTS::t_data_consumer() {

        // Elemento a reproducir de la cola
        queueElement element;

        while(running_) {

            // Salir si el programa se está cerrando (antes)
            if (!running_) break;

            // Forzar espera hasta que haya algo en la cola o se cierre este player
            std::unique_lock<std::mutex> lock(cola_textos_mtx_);
            cola_textos_cv_.wait(lock, [this] {
                return !running_ || !cola_textos_.empty();
            });

            // Salir si el programa se está cerrando (después)
            if(!running_) break;

            // Obtener tarea de la cola
            element = cola_textos_.front();
            cola_textos_.pop();

            // Libera el lock de aquí en adelante
            lock.unlock();

            // Reproducir elemento
            reproducir_elemento(element);

            // Esperar un rato hasta reproducir el siguiente paquete
            std::this_thread::sleep_for(std::chrono::seconds(2));   // (poner tiempo como variable)
        }

        SYS_INFO("PlayerTTS::t_data_consumer", "Thread stopped.");

    }


// Procesado de elementos de la cola ----------------------------------------------------

    bool PlayerTTS::reproducir_elemento(queueElement element) {

        // Guardar el texto que se está procesando
        texto_en_proceso_ = element.text;

        // Generar audio (muestras + sample rate real del modelo usado)
        AudioData audio = onTextToAudio_cb_(element.modelName, element.text);
        if (audio.samples.empty()) {
            SYS_WARN("PlayerTTS", "Empty audio generated from " + element.modelName);
            texto_en_proceso_.clear();
            return false;
        }

        // Reproducir audio por el playback

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

        // Copiar los datos generados a un buffer propio de miniaudio
        if (ma_audio_buffer_init_copy(&config, &inst->buffer) != MA_SUCCESS) {
            SYS_WARN("PlayerTTS", "reproducir_elemento: fallo al crear el buffer de audio");
            texto_en_proceso_.clear();
            return false;
        }
        inst->isBuffer = true;

        // Envolver el buffer como un sonido reproducible por el motor
        if (ma_sound_init_from_data_source(&pimpl_->engine, &inst->buffer, 0, nullptr, &inst->sound) != MA_SUCCESS) {
            ma_audio_buffer_uninit(&inst->buffer);
            SYS_WARN("PlayerTTS", "reproducir_elemento: fallo al inicializar el sonido");
            texto_en_proceso_.clear();
            return false;
        }

        // Establecer parámetros de la reproducción
        float ma_volume = (static_cast<float>(element.volume) / 100.0f) * static_cast<float>(globalVol_) / 100.0f;
        ma_sound_set_volume(&inst->sound, ma_volume);  // (va de 0.0 a 1.0)
        ma_sound_set_looping(&inst->sound, (element.loop) ? MA_TRUE : MA_FALSE);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        // Guardar parámetros en la instancia de sonido
        inst->loopMode  = element.loop;
        inst->forceStop = element.forceStop;
        inst->name      = element.audioName;
        inst->volume    = element.volume;

        // Guardar en el mapa y reproducir
        {
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[element.audioName] = std::move(inst);

            SYS_INFO("PlayerTTS","'" + element.audioName + "': init playing TTS...");
            ma_sound_start(&pimpl_->playing_sounds[element.audioName]->sound);
        }

        // Limpiar el texto que se está procesando
        texto_en_proceso_.clear();

        // Notificar que ha terminado de reproducir (iComm)
        /* #TODO */

        return true;
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
