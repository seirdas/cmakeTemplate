#include "sound/AudioInputModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <thread>               // Hilos
    #include <cmath>
    #include "system/SystemMgr.hpp"
    #include <cmath>

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
        self->buffer_.insert(self->buffer_.end(), samples, samples + frameCount * self->channels_);
        if (self->buffer_.size() >= self->processBufferSize_ * self->channels_){

            // Obtiene valor del RMS
            float sum = 0.0f; // acumulador de la suma de cuadrado
            for (ma_uint32 i = 0; i < frameCount * self->channels_; ++i) 
            {
                sum += (float)samples[i] * samples[i]; // eleva cada muestra al cuadrado y acumula

                // divide la suma entre el número de muestras → media de cuadrados → raíz cuadrada → RMS
                self->rmsLevel_ = std::sqrt(sum / (frameCount * self->channels_)) / 327.67f; // Normaliza entre 0 y 100
            }
            self->buffer_.clear();
        }


        // 2. GRABACIÓN: Guarda los samples en el buffer de grabación si está grabando. Cada frame tiene una muestra por canal
        if (self->recording_) {
            self->rec_buffer_.insert(self->rec_buffer_.end(), samples, samples + frameCount * self->channels_);
        }

        // 3. CALLBACK: Envío de trama de datos de audio de entrada a "otro sitio" si el callback está definido
        if (self->onFrame_ != nullptr) {
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
        sampleRate_(44100),
        processBufferSize_(1024),
        codec_inited_(false),
        recording_(false),
        is_valid_(false),
        rmsLevel_(0.0f),
        onFrame_(nullptr)
    {
        deviceName_ = pimpl_->device_info.name;
    }

    AudioInputModule::~AudioInputModule() {
        stop();
    }


    // Ejecución-----------------------------------------------------------------------------

    bool AudioInputModule::init() {
        if (is_valid_) return true; 
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

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

    void AudioInputModule::stop() {

        // Si estaba grabando, parar la grabación (no guardamos nada)
        if (recording_) {
            SYS_WARN("AudioInputModule","Device stopped while recording.");
            StopRec();
            rec_buffer_.clear();
        }

        // Detener la captura de audio
        ma_device_stop(&pimpl_->device);

        // Desinicializa el dispositivo
        ma_device_uninit(&pimpl_->device);
        is_valid_ = false;
    }


    // Información y parámetros -------------------------------------------------------------

    std::string AudioInputModule::deviceName() const { 
        return deviceName_; 
    }

    bool AudioInputModule::isValid() {
        return is_valid_;
    };

    // Captura ------------------------------------------------------------------------------

    float AudioInputModule::getRmsLevel() const { 
        return rmsLevel_; 
    }
    
    size_t AudioInputModule::getBufferSize() {
        return buffer_.size();
    };


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
AudioInputModule::~AudioInputModule() {}

// Ejecución -----------------------------------------------------------------------------
bool AudioInputModule::init() { return false; }
void AudioInputModule::stop() { return; }

// Información y parámetros --------------------------------------------------------------
std::string AudioInputModule::deviceName() const { return ""; }
bool AudioInputModule::isValid()                 { return false; }
float AudioInputModule::getRmsLevel() const      { return 0.0f; }
size_t AudioInputModule::getBufferSize()         { return 0; }

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