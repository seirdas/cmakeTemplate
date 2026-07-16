#include "sound/AudioInputModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <cmath>
    #include <limits>
    #include "system/SystemMgr.hpp"
    #include <files/JsonMgr.hpp>

    // Implementación de miembros y métodos de la librería externa
    struct AudioInputModule::Impl {
        ma_context*    ctx = nullptr;   ///< Contexto de audio
        ma_device      device;          ///< Dispositivo de entrada
        ma_device_info device_info;     ///< Información del dispositivo de entrada
        ma_encoder     encoder;         ///< Codificador de audio para grabación 

        /**
         * @brief Constructor de Impl
         *  El constructor del Impl hace el cast de los punteros opacos
         * @param context (ma_context*) Contexto de audio de miniaudio
         * @param devInfo (ma_device_info*) Información del dispositivo
         */
        Impl(void* context, const void* devInfo);

        /**
         * @brief Miniaudio llama a esta función cuando el estado del dispositivo cambia 
         * @note Está definido en el Impl porque la firma de la función debe ser así, y depende de la librería
         */
        static void notificationCallback_(const ma_device_notification* pNotification);

        /** 
         * @brief Miniaudio llama a esta función automáticamente cada vez que el micrófono captura un sample de audio
         * @note Está definido en el Impl porque la firma de la función debe ser así, y depende de la librería
         */
        static void dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);
    };

    // Implementación de métodos PIMPL ------------------------------------------------------

    AudioInputModule::Impl::Impl(void* context, const void* devInfo) {
        ctx = static_cast<ma_context*>(context);
        device_info = *static_cast<const ma_device_info*>(devInfo);
    }

    void AudioInputModule::Impl::dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) 
    {
        // Recupera el puntero a this, así el CallBack puede acceder a los miembros de la clase. 
        AudioInputModule* self = static_cast<AudioInputModule*>(pDevice->pUserData);

        // pInput llega sin tipo y lo convierte a int16_t (formato que se ha configurado ma_format_s16)
        const int16_t* samples = static_cast<const int16_t*>(pInput);

        // 1. NIVEL DE SEÑAL: Procesar frame de muestras de captura (normales) y limpiar buffer cuando se llene
        self->captureBuffer_.insert(self->captureBuffer_.end(), samples, samples + frameCount * self->channels_);

        // Procesamos solo cuando el buffer acumulado alcance el tamaño deseado
        ma_uint32 targetSize = self->processBufferSize_ * self->channels_;
        if (self->captureBuffer_.size() >= targetSize) {

            /* Valor de pico del buffer */
            int32_t peak = 0;       // 32 bits para evitar overflow con el valor -32768 
            int32_t sampleAbs = 0;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleAbs = std::abs(static_cast<int32_t>(self->captureBuffer_[i]));
                if (sampleAbs > peak)
                    peak = sampleAbs;
            }
            self->peakLevel_ = static_cast<float>((peak / static_cast<float>(self->max_int16_val_)) * 100.0f); //de 0 a 100


            /* Valor RMS */
            double sampleVal = 0.0f;
            double sum = 0.0f;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleVal = static_cast<double>(self->captureBuffer_[i]);
                sum += sampleVal * sampleVal;
            }
            double rms = std::sqrt(sum / targetSize); // La raiz es más eficiente hacerla fuera del bucle
            self->rmsLevel_  = static_cast<float>((rms  / self->max_int16_val_) * 100.0); // de 0 a 1
            if (self->peakLevel_ > 100.0f) self->peakLevel_ = 100.0f;
        
            /* Limpieza de buffer */
            self->captureBuffer_.clear();
        }

        // 2. GRABACIÓN: Guarda los samples en el buffer de grabación si está grabando. Cada frame tiene una muestra por canal
        if (self->recording_)
            self->rec_buffer_.insert(self->rec_buffer_.end(), samples, samples + frameCount * self->channels_);

        // 3. CALLBACK: Envío de trama de datos de audio de entrada a "otro sitio" si el callback está definido
        {
            std::lock_guard<std::mutex> lk(self->onFrame_mtx_);
            if (self->onFrame_ != nullptr)
                self->onFrame_(samples, frameCount);
        }
    }

    void AudioInputModule::Impl::notificationCallback_(const ma_device_notification* pNotification) {

        AudioInputModule* self = static_cast<AudioInputModule*>(pNotification->pDevice->pUserData);

        // Avisar, guardar y notificar si el dispositivo se ha desconectado o ya no está disponible
        if (pNotification->type == ma_device_notification_type_stopped) {
            self->is_valid_ = false;
            SYS_WARN("AudioInputModule", "Device disconnected or stopped unexpectedly.");
        }
        else {
            self->is_valid_ = true;
            SYS_INFO("AudioInputModule", "Device connected.");
        }
    }



    // General ------------------------------------------------------------------------------

    AudioInputModule::AudioInputModule(void* ctx, const void* devInfo) :
    pimpl_(std::make_unique<Impl>(ctx, devInfo)),
    channels_(2),
    sampleRate_(48000),
    processBufferSize_(1024),
    codec_inited_(false),
    recording_(false),
    is_valid_(false),
    onFrame_(nullptr),
    max_int16_val_(std::numeric_limits<int16_t>::max())
    {
        device_ = pimpl_->device_info.name;
    }

    AudioInputModule::~AudioInputModule() {
        stop();
    }


    // Ejecución-----------------------------------------------------------------------------

    bool AudioInputModule::init(void* config) {
        
        if (is_valid_) return true; 
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else  // Puede llegar aquí cuando se hace reload()
            SYS_WARN("AIM","Cannot load config. Using default values.");

        deviceConfig.capture.format       = ma_format_s16;
        deviceConfig.capture.channels     = channels_;
        deviceConfig.sampleRate           = sampleRate_;
        deviceConfig.dataCallback         = Impl::dataCallback_;
        deviceConfig.notificationCallback = Impl::notificationCallback_;
        deviceConfig.pUserData            = this;             
        deviceConfig.capture.pDeviceID    = &pimpl_->device_info.id;

        if (ma_device_init(pimpl_->ctx, &deviceConfig, &pimpl_->device) != MA_SUCCESS) {
            SYS_WARN("AudioInputModule","Cannot initialize input device");
            return false;
        }

        if (ma_device_start(&pimpl_->device) != MA_SUCCESS){
            SYS_WARN("AudioInputModule","Cannot start input device");
            ma_device_uninit(&pimpl_->device);
            return false;
        }

        // Llega hasta aquí si se ha inicializado bien
        is_valid_ = true;
        return true;
    }
    
    void AudioInputModule::loadConfig(void* config) {
        if (!config)
             return;

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        jsonMgr.get_or_set(cfg, "name", name_);
        
        /* Esto ya llega en la inicialización, en devInfo del constructor */
        //jsonMgr.get_or_set(cfg, "device", device_);
        
        jsonMgr.get_or_set(cfg, "numchannels", channels_);
        jsonMgr.get_or_set(cfg, "sample_rate", sampleRate_);
        jsonMgr.get_or_set(cfg, "process_buffer_size", processBufferSize_);

    }

    void AudioInputModule::stop() {

        // Si estaba grabando, parar la grabación (no guardamos nada)
        if (recording_) {
            SYS_WARN("AudioInputModule","Device stopped while recording.");
            StopRec();
            rec_buffer_.clear();
        }

        // Detener la captura de audio
        SYS_INFO("AudioInputModule","Stopping device...");
        ma_device_stop(&pimpl_->device);

        // Desinicializa el dispositivo
        SYS_INFO("AudioInputModule","Uninit device...");
        ma_device_uninit(&pimpl_->device);
        is_valid_ = false;

        // Limpia el callback
        SYS_INFO("AudioInputModule","Clearing onframe callback injected...");
        clearOnFrameCallback();
    }


    // Información y parámetros -------------------------------------------------------------

    std::string AudioInputModule::deviceName() const { 
        return device_; 
    }

    bool AudioInputModule::isValid() {
        return is_valid_;
    };


    // Captura ------------------------------------------------------------------------------

    float AudioInputModule::getRmsLevel() const { 
        return rmsLevel_; 
    }

    float AudioInputModule::getPeakLevel() const { 
        return peakLevel_; 
    }
    
    size_t AudioInputModule::getBufferSize() {
        return captureBuffer_.size();
    };


    // Callback expuesto --------------------------------------------------------------------
    
    void AudioInputModule::setOnFrameCallback(AudioCallback cb) {
        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_ = std::move(cb); 
    }

    void AudioInputModule::clearOnFrameCallback() {
        if (!onFrame_) 
            return;

        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_ = nullptr;
    }


    // Grabación ----------------------------------------------------------------------------

    void AudioInputModule::StartRec(std::string const& filename) {

        // Inicializar el encoder (necesita el nombre de archivo)
        InitRecEncoder(filename);

        // Flag para activar la toma de samples en el buffer de grabación del callback de captura
        recording_ = true;
    }

    void AudioInputModule::StopRec() {

        // Si no está grabando, no hacer nada
        if (!recording_) return;

        // Flag para desactivar la toma de samples en el buffer de grabación del callback de captura
        recording_=false; 

        // Guarda las muestras grabadas en el buffer de grabación en el archivo
        saveRecording();

        // Desinicializa el encoder
        UninitRecEncoder();
    }

    size_t AudioInputModule::getRecBufferSize() {
        return rec_buffer_.size();
    };

    bool AudioInputModule::isRecording() {
        return recording_; 
    }


    // Codificador de grabación -------------------------------------------------------------

    void AudioInputModule::InitRecEncoder(std::string const& filename) {

        // Le dices cómo quieres que sea el archivo.
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,  // formato WAV
            ma_format_s16,           // mismo formato que la captura
            channels_,               // mismos canales
            sampleRate_              // mismo sample rate
        );

        // Crea el archivo en el disco y prepara el encoder_ para escribir en él
        ma_encoder_init_file((filename + ".wav").c_str(), &config, &pimpl_->encoder);

        // Guarda el nombre para usarlo después
        rec_filename_=filename;
    }

    void AudioInputModule::UninitRecEncoder() {
        ma_encoder_uninit(&pimpl_->encoder);
    }

    void AudioInputModule::saveRecording() {

        // Variable donde miniaudio guarda cuantos frames ha escrito realmente
        ma_uint64 framesWritten = 0;
        
        // Volcado a disco
        ma_encoder_write_pcm_frames(&pimpl_->encoder, rec_buffer_.data(), rec_buffer_.size() / channels_, &framesWritten);

        // Mostrar info del tiempo grabado y el archivo de salida
        float recTime_s = static_cast<float>(framesWritten) / static_cast<float>(sampleRate_);
        SYS_INFO("AudioInputModule","Record " + std::to_string(recTime_s) + "s to file '" + rec_filename_ + "'");
        rec_filename_.clear();

        // Limpiar el buffer de grabación
        rec_buffer_.clear();
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
AudioInputModule::AudioInputModule(void*, const void*) {}
AudioInputModule::~AudioInputModule()   { return; }

// Ejecución -----------------------------------------------------------------------------
bool AudioInputModule::init(void* config) { return false; }
void AudioInputModule::stop()             { return; }
void AudioInputModule::loadConfig()     { return; }

// Información y parámetros --------------------------------------------------------------
std::string AudioInputModule::deviceName() const { return ""; }
bool AudioInputModule::isValid()                 { return false; }

// Captura ------------------------------------------------------------------------------
float  AudioInputModule::getRmsLevel() const    { return 0.0f;  }
size_t AudioInputModule::getBufferSize()        { return 0;     }
float  AudioInputModule::getPeakLevel() const   { return 0;     }

// Callback expuesto --------------------------------------------------------------------
void AudioInputModule::setOnFrameCallback(AudioCallback cb);    { return; }

// Grabación ----------------------------------------------------------------------------
void AudioInputModule::StartRec(std::string const&) { return; }
void AudioInputModule::StopRec()                    { return; }
size_t AudioInputModule::getRecBufferSize()         { return 0; }
bool AudioInputModule::isRecording()                { return false; }

// Codificador de grabación (Privados) --------------------------------------------------
void AudioInputModule::InitRecEncoder(std::string const&) { return; }
void AudioInputModule::UninitRecEncoder()                 { return; }
void AudioInputModule::saveRecording()                    { return; }

#endif