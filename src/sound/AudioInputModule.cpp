#include "sound/AudioInputModule.hpp"


// Macro de cmake al activar la librería
#if defined MINIAUDIO || defined MINIAUDIO_VERSION
    #include <cmath>
    #include "system/SystemMgr.hpp"

    AudioInputModule::AudioInputModule(ma_context* ctx, ma_device_info const& device_info):
        recording_(false),
        is_valid_(false),
        ctx_(ctx),
        device_info_(device_info),
        deviceName_(device_info.name)
    {

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
        deviceConfig.dataCallback         = dataCallback_;
        deviceConfig.notificationCallback = notificationCallback_;
        deviceConfig.pUserData            = this;             
        deviceConfig.capture.pDeviceID    = &device_info_.id;

        if (ma_device_init(ctx_, &deviceConfig, &device_) != MA_SUCCESS) {
            SYS_WARN("AudioInputModule","Cannot initialize input device");
            return false;
        }

        if (ma_device_start(&device_) != MA_SUCCESS){
            SYS_WARN("AudioInputModule","Cannot start input device");
            ma_device_uninit(&device_);
            return false;
        }

        // Llega hasta aquí si se ha inicializado bien
        is_valid_ = true;
        return true;
    }

    void AudioInputModule::stop() {
        ma_device_stop(&device_);    // para de capturar audio
        ma_device_uninit(&device_);  // libera el dispositivo
        is_valid_ = false;
    }


    // Audio---------------------------------------------------------------------------------

    bool  AudioInputModule::StartRec() {

        // El callback vacía el buffer para meter datos nuevos
        rec_buffer_.clear();

        // Si está en true los guarda, si está en false los borra
        recording_ = true;

        return true; 
    }

    bool AudioInputModule::StopRec() {

        // En el momento recording está en false, para de grabar
        recording_=false; 
        return true;  
    }

    bool AudioInputModule::isRecording() {
        //Devuelve el valor del recording true o false, 
        //depende de si está en startrec o stoprec
        return recording_; 
    }


    // Grabacion ----------------------------------------------------------------------------

    void AudioInputModule::InitwavEncoder(const std::string& filename) {

        // Le dices cómo quieres que sea el archivo.
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,  // formato WAV
            ma_format_s16,           // mismo formato que la captura
            channels_,               // mismos canales
            sampleRate_              // mismo sample rate
        );

        // Crea el archivo en el disco y prepara el encoder_ para escribir en el 
        ma_encoder_init_file(filename.c_str(), &config, &encoder_);
    }

    void AudioInputModule::RemovewavEncoder() {
        //Funcion de miniaudio que cierra el encoder
        ma_encoder_uninit(&encoder_);
    }

    void AudioInputModule::saveSound() {

        // Variable donde miniaudio guarda cuantos frames ha escrito realmente
        ma_uint64 framesWritten;

        // Puntero al inicio del buffer con todas las muestras grabadas
        // Convierte muestras a frames. Si tienes 88200 muestras con 2 canales, son 44100 frames
        ma_encoder_write_pcm_frames(&encoder_, rec_buffer_.data(), rec_buffer_.size() / channels_, &framesWritten);
    }

    size_t AudioInputModule::getBufferSize() {
        return buffer_.size();
    };

    size_t AudioInputModule::getRecBufferSize() {
        return rec_buffer_.size();
    };

    bool AudioInputModule::isValid() {
        return is_valid_;
    };


    // CallBacks ----------------------------------------------------------------------------

    void AudioInputModule::dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) 
    {

        // Recupera el puntero a this, así el CallBack puede acceder a los miembros de la clase. 
        AudioInputModule* self = static_cast<AudioInputModule*>(pDevice->pUserData);

        // pInput llega sin tipo y lo convierte a int16_t (formatpo que se ha configurado ma_format_s16)
        const int16_t* samples = static_cast<const int16_t*>(pInput);

        // Procesar frame de muestras de captura (normales) y borrar
        self->buffer_.insert(self->buffer_.end(), samples, samples + frameCount * self->channels_);

        // Si buffer llega a 1024 muestras se resetea
        if (self->buffer_.size() >= self->processBufferSize_ * self->channels_){

            // Obtiene valor del RMS
            float sum = 0.0f; // acumulador de la suma de cuadrado
            for (ma_uint32 i = 0; i < frameCount * self->channels_; ++i) 
            {
                sum += (float)samples[i] * samples[i]; // eleva cada muestra al cuadrado y acumula

                // divide la suma entre el número de muestras → media de cuadrados → raíz cuadrada → RMS
                // divide entre 32767 para normalizar de int16 a rango 0.0-1.0
                self->rmsLevel_ = std::sqrt(sum / (frameCount * self->channels_)) / 327.67f; // Normaliza entre 0 y 100
            } 
            SYS_INFO("AudioInputModule", "Valor RMS: " + std::to_string(self->rmsLevel_));

            self->buffer_.clear();
        }

        // Si no estás grabando, ignora los datos y sale.
        if (!self->recording_) return;

        // Mete las muestras nuevas al final del buffer. 
        // Cada frame tiene una muestra por canal (2 canales = 2 valores por frame).
        self->rec_buffer_.insert(self->rec_buffer_.end(), samples, samples + frameCount * self->channels_);
    }

    void AudioInputModule::notificationCallback_(const ma_device_notification* pNotification) {

        AudioInputModule* self = static_cast<AudioInputModule*>(pNotification->pDevice->pUserData);
        if (pNotification->type == ma_device_notification_type_stopped) {
            self->is_valid_ = false;
            SYS_WARN("AudioInputModule", "Device disconnected or stopped unexpectedly.");
        }
        else {
            self->is_valid_ = true;
            SYS_INFO("AudioInputModule", "Device connected.");
        }


    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
AudioInputModule::AudioInputModule(ma_context* ctx, ma_device_info const& device_info)  {}
AudioInputModule::~AudioInputModule()                                                   {}

// Ejecución-----------------------------------------------------------------------------
bool AudioInputModule::init()                      {return false;}
void AudioInputModule::stop()                      {return; }

// Audio---------------------------------------------------------------------------------
bool  AudioInputModule::StartRec()                 {return false; }
bool AudioInputModule::StopRec()                   {return false; }
bool AudioInputModule::isRecording()               {return false; }


// Grabacion ----------------------------------------------------------------------------
void AudioInputModule::InitwavEncoder(const std::string& )   {return; }
void AudioInputModule::RemovewavEncoder()                    {return; }
void AudioInputModule::saveSound()                           {return; }
size_t AudioInputModule::getBufferSize()                     {return 0; }
size_t AudioInputModule::getRecBufferSize()                  {return 0; }
bool AudioInputModule::isValid()                             {return false; }


// CallBacks ----------------------------------------------------------------------------
void AudioInputModule::dataCallback_(ma_device* , void* , const void* , ma_uint32 ) {return; }
void AudioInputModule::notificationCallback_(const ma_device_notification*)         {return; }

#endif