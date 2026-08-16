#include "sound/AudioInputModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <cmath>
    #include <limits>
    #include "system/SystemMgr.hpp"
    #include <files/JsonMgr.hpp>

    // Implementación de miembros y métodos de la librería externa
    struct AudioInputModule::Impl {
        ma_context*    ctx = nullptr;   ///< Contexto de miniaudio
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

    void AudioInputModule::Impl::dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) {
        // Recupera el puntero a this, así el CallBack puede acceder a los miembros de la clase. 
        AudioInputModule* self = static_cast<AudioInputModule*>(pDevice->pUserData);

        // Recoge los frames capturados en este callback (samples en frameCount)
        const int16_t* samples = static_cast<const int16_t*>(pInput);

        // Filtrar a las muestras del canal seleccionado (si aplica, osea, si channelSelected=0)
        bool captureAll = (self->selectedChannel_ == 0);
        std::vector<int16_t> filteredSamples;
        unsigned int numSamples = (captureAll) ? frameCount * self->channels_ : frameCount;
        filteredSamples.reserve(numSamples);

        if (captureAll)
            // Opción 1: Guarda todos los samples de todos los canales
            filteredSamples.assign(samples, samples + numSamples);
        else {
            // Opción 2: Filtrar un canal específico
            // Ajustamos el índice: si selecciona 1, restamos 1 para acceder al índice 0
            int idx = self->selectedChannel_ - 1; 
            
            // Protegemos contra índices inválidos
            if (idx >= 0 && idx < static_cast<int>(self->channels_))
                for (unsigned int i = 0; i < frameCount; ++i)
                    filteredSamples.push_back(samples[i * self->channels_ + idx]);
            else {
                // Fallback por si fallo, grabar todos los canales
                SYS_WARN("AudioInputModule","Bad channel selection: capturing all device channels");
                filteredSamples.assign(samples, samples + numSamples);
            }
        }

        // 1. NIVEL DE SEÑAL: Procesar frame de muestras de captura (normales) y limpiar buffer cuando se llene
        self->captureBuffer_.insert(self->captureBuffer_.end(), filteredSamples.begin(), filteredSamples.end());

        // Procesamos solo cuando el buffer acumulado alcance el tamaño deseado
        ma_uint32 targetSize = self->processBufferSize_ * (captureAll ? self->channels_ : 1);
        if (self->captureBuffer_.size() >= targetSize) {

            // Variable temporal para almacenar valores
            float rawValue = 0;


            /* Valor de pico del buffer (0-100)*/
            int32_t peak = 0;       // 32 bits para evitar overflow con el valor -32768 
            int32_t sampleAbs = 0;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleAbs = std::abs(static_cast<int32_t>(self->captureBuffer_[i]));
                if (sampleAbs > peak)
                    peak = sampleAbs;
            }
            rawValue = static_cast<float>((peak / static_cast<float>(self->max_int16_val_)) * 100.0f); //de 0 a 100
            self->peakLevel_ = (self->smoothedValues_) ? self->smooth_level(rawValue, self->peakLevel_) : rawValue;
            if (self->peakLevel_ > 100.0f) self->peakLevel_ = 100.0f; // capar al máximo
            

            /* Valor RMS (0-100)*/
            double sampleVal = 0.0f;
            double sum = 0.0f;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleVal = static_cast<double>(self->captureBuffer_[i]);
                sum += sampleVal * sampleVal;
            }
            double rms = std::sqrt(sum / targetSize); // La raiz es más eficiente hacerla fuera del bucle
            rawValue  = static_cast<float>((rms  / self->max_int16_val_) * 100.0); // de 0 a 100
            self->rmsLevel_ = (self->smoothedValues_) ? self->smooth_level(rawValue, self->rmsLevel_) : rawValue;
            if (self->rmsLevel_ > 100.0f) self->rmsLevel_ = 100.0f;

        
            /* Limpieza de buffer */
            self->captureBuffer_.clear();
        }

        // 2. GRABACIÓN: Guarda los samples en el buffer de grabación si está grabando. Cada frame tiene una muestra por canal
        if (self->recording_)
            self->rec_buffer_.insert(self->rec_buffer_.end(), filteredSamples.begin(), filteredSamples.end());

        // 3. CALLBACK: Envío de trama de datos de audio de entrada a "otro sitio" si el callback está definido
        {
            std::lock_guard<std::mutex> lk(self->onFrame_mtx_);
            if (self->onFrame_cb_ != nullptr) {
                self->onFrame_cb_(filteredSamples.data(), filteredSamples.size());
            }
        }
    }

    void AudioInputModule::Impl::notificationCallback_(const ma_device_notification* pNotification) {

        AudioInputModule* self = static_cast<AudioInputModule*>(pNotification->pDevice->pUserData);

        // Avisar, guardar y notificar si el dispositivo se ha desconectado o ya no está disponible
        if (pNotification->type == ma_device_notification_type_stopped) {
            self->is_valid_ = false;
            if (self->running_) // Avisa si no se está cerrando
                SYS_WARN("AudioInputModule", "Device disconnected or stopped unexpectedly.");
        }
        else {
            self->is_valid_ = true;
            SYS_INFO("AudioInputModule", "Device connected.");
        }
    }



    // General ------------------------------------------------------------------------------

    AudioInputModule::AudioInputModule(void* ctx, const void* device_info) :
    pimpl_(std::make_unique<Impl>(ctx, device_info)),
    is_valid_(false),
    initialized_(false),
    running_(false),
    channels_(2),
    sampleRate_(48000),
    selectedChannel_(0),
    codec_inited_(false),
    recording_(false),
    max_int16_val_(std::numeric_limits<int16_t>::max()),
    processBufferSize_(1024),
    smoothedValues_(false),
    attackCoeff_(0),
    releaseCoeff_(0),
    onFrame_cb_(nullptr)
    {
        deviceName_ = pimpl_->device_info.name;
    }

    AudioInputModule::~AudioInputModule() {
        close();
    }


    // Inicialización -------------------------------------------------------------------

    bool AudioInputModule::init(void* config, std::string const& captureName) {
        if (initialized_)
            return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

        // Fallo si selectedchannels no está dentro de numChannels
        if (selectedChannel_ > channels_) {
            SYS_WARN("AudioInputModule","Cannot initialize channel: selected:" 
                + std::to_string(selectedChannel_) + ", channels: " + std::to_string(channels_));
            return false;
        }

        // Ponerle nombre a este módulo (sobreescribe el de la config)
        if (!captureName.empty()) name_ = captureName;

        // Inicializar el dispositivo de captura
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

        // rellenar los parámetros de la configuración de miniaudio
        deviceConfig.capture.format       = ma_format_s16;
        deviceConfig.capture.channels     = static_cast<unsigned int>(channels_);
        deviceConfig.sampleRate           = sampleRate_;
        deviceConfig.dataCallback         = Impl::dataCallback_;
        deviceConfig.notificationCallback = Impl::notificationCallback_;
        deviceConfig.pUserData            = this;             
        deviceConfig.capture.pDeviceID    = &pimpl_->device_info.id;

        // Inicializar
        if (ma_device_init(pimpl_->ctx, &deviceConfig, &pimpl_->device) != MA_SUCCESS) {
            SYS_WARN("AudioInputModule","Cannot initialize input device");
            is_valid_ = false;
            return false;
        }

        // Comenzar a capturar audio
        running_ = startCapture();

        // Llega hasta aquí si se ha inicializado bien
        initialized_ = true;
        return initialized_; //<- true
    }

    bool AudioInputModule::isInitialized() const {
        return initialized_;
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
        
        jsonMgr.get_or_set(cfg, "numchannels",          channels_);
        jsonMgr.get_or_set(cfg, "sample_rate",          sampleRate_);
        jsonMgr.get_or_set(cfg, "process_buffer_size",  processBufferSize_);
        jsonMgr.get_or_set(cfg, "selected_channel",     selectedChannel_);

    }

    void AudioInputModule::close() {

        // Si no está inicializado, no hacer nada
        if (!initialized_)
            return;

        // Marco running false para que no salga warn de "input closed unexpectedly"
        running_ = false;

        // Parar la captura
        SYS_INFO("AudioInputModule","Stopping device...");
        stopCapture();

        // Desinicializa el dispositivo
        SYS_INFO("AudioInputModule","Uninit device...");
        ma_device_uninit(&pimpl_->device);
        is_valid_ = false;

        // Limpia el callback
        SYS_INFO("AudioInputModule","Clearing onframe callback injected...");
        clearCallback_OnFrame();

        initialized_ = false;
    }

    bool AudioInputModule::reload() {

        SYS_INFO("AudioInputModule", "Reloading module...");
        
        // Parar todo si está inicializado
        if (initialized_)
            close();

        // Inicializa tomando los parámetros nuevos para la config (samplerate, channels, etc.)
        return init();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool AudioInputModule::startCapture() {
        if (running_)
            return true;

        SYS_INFO("AudioInputModule","Initializing capture...");
        if (ma_device_start(&pimpl_->device) != MA_SUCCESS){
            SYS_WARN("AudioInputModule","Cannot start input device");
            ma_device_uninit(&pimpl_->device);
            return false;
        }
        else return true;
    }

    bool AudioInputModule::stopCapture() {
        if (!running_)
            return true;

        // Si estaba grabando, parar la grabación (se guarda lo que hubiera)
        if (recording_) {
            SYS_WARN("AudioInputModule","Device stopped while recording.");
            StopRec();
            rec_buffer_.clear();
        }

        // Detener la captura de audio
        SYS_INFO("AudioInputModule","Stopping device...");
        ma_result res = ma_device_stop(&pimpl_->device);
        return res == MA_SUCCESS;
    }


    // Parámetros del módulo ----------------------------------------------------------------

    std::string AudioInputModule::getDeviceName() const { 
        return pimpl_->device_info.name; 
    }

    std::string AudioInputModule::getModuleName() const {
        return name_;
    }

    unsigned short AudioInputModule::getNumChannels() const {
        return channels_;
    }

    unsigned short AudioInputModule::getSelectedChannel() const {
        return selectedChannel_;
    }

    unsigned int AudioInputModule::getSampleRate() const {
        return sampleRate_;
    }

    bool AudioInputModule::setDeviceName(std::string const& deviceName) {

        // Obtener el ma_device_info a partir del nombre
        ma_device_info* selectedDeviceInfo  = nullptr;
        unsigned int    captureDevCount     = 0;
        ma_device_info* pCaptureDevInfos   = nullptr;

        // Obtener los dispositivos de captura (simplificado)
        ma_result res = ma_context_get_devices(pimpl_->ctx,
            nullptr, nullptr, 
            &pCaptureDevInfos, &captureDevCount);

        for (unsigned int i = 0; i < captureDevCount; ++i)
            if (deviceName == pCaptureDevInfos[i].name) {  
                selectedDeviceInfo = &pCaptureDevInfos[i];
                break;
            }
        // No hace comprobación de duplicados ni inferencia de nombre

        // Si no encuentra ningún nombre salta fallo
        if(!selectedDeviceInfo){
            SYS_WARN("AudioInputModule", "Failed to found device: '" + deviceName + "'"); 
            return false; 
        }

        // Guarda el dispositivo encontrado
        pimpl_->device_info = *selectedDeviceInfo;

        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioInputModule::setNumChannels(unsigned short numChannels) {
        channels_ = numChannels;
        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioInputModule::setSampleRate(unsigned int sampleRate) {
        sampleRate_ = sampleRate;
        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioInputModule::setSelectedChannel(unsigned short selectedChannel) {
        if (selectedChannel > channels_) {
            SYS_WARN("AudioInputModule","Cannot change channel: selected:" 
                + std::to_string(selectedChannel) + ", channels: " + std::to_string(channels_));
                return false;
        }
        selectedChannel_ = selectedChannel;
        return true;
    }

    bool AudioInputModule::isValid() const {
        return is_valid_;
    };


    // Captura ------------------------------------------------------------------------------

    float AudioInputModule::getRmsLevel() const { 
        return rmsLevel_; 
    }

    float AudioInputModule::getPeakLevel() const { 
        return peakLevel_; 
    }
    
    size_t AudioInputModule::getBufferSize() const {
        return captureBuffer_.size();
    };


    // Callback expuesto --------------------------------------------------------------------
    
    void AudioInputModule::setCallback_OnFrame(AudioCallback cb) {
        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_cb_ = std::move(cb); 
    }

    void AudioInputModule::clearCallback_OnFrame() {
        if (!onFrame_cb_) return;
        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_cb_ = nullptr;
    }

    bool AudioInputModule::hasCallback_OnFrame() {
        return static_cast<bool>(onFrame_cb_);
    }


    // Grabación ----------------------------------------------------------------------------

    bool AudioInputModule::StartRec(std::string const& filename) {
        // Inicializar el encoder (necesita el nombre de archivo) con 
        // flag para activar la toma de samples en el buffer de grabación del callback de captura
        recording_ = init_rec_encoder(filename);
        return recording_;
    }

    bool AudioInputModule::StopRec() {

        // Si no está grabando, no hacer nada
        if (!recording_) return false;

        // Flag para desactivar la toma de samples en el buffer de grabación del callback de captura
        recording_=false; 

        // Guarda las muestras grabadas en el buffer de grabación en el archivo
        bool res = save_recording();

        // Desinicializa el encoder
        uninit_rec_encoder();

        // Devuelve el resultado de save_recording
        return res;
    }

    size_t AudioInputModule::getRecBufferSize() const {
        return rec_buffer_.size();
    };

    bool AudioInputModule::isRecording() const {
        return recording_; 
    }


    // Parámetros de suavizado de valores ---------------------------------------------------

    void AudioInputModule::enableSmoothedValues(bool value) {
        smoothedValues_ = value;
    }

    void AudioInputModule::setSmoothAttackCoeff(float value) {
        attackCoeff_ = value;
    }

    void AudioInputModule::setSmoothReleaseCoeff(float value) {
        releaseCoeff_ = value;
    }


    // Codificador de grabación -------------------------------------------------------------

    bool AudioInputModule::init_rec_encoder(std::string const& filename) {

        // Le dices cómo quieres que sea el archivo.
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,  // formato WAV
            ma_format_s16,           // mismo formato que la captura
            channels_,               // mismos canales
            sampleRate_              // mismo sample rate
        );

        // Crea el archivo en el disco y prepara el encoder_ para escribir en él
        ma_result res = ma_encoder_init_file((filename + ".wav").c_str(), &config, &pimpl_->encoder);

        // Comprobar si se ha inicializado bien el encoder
        if (res != MA_SUCCESS) {
            SYS_WARN("AudioInputModule","init_rec_encoder: ma_encoder_init_file error");
            return false;
        }

        // Guarda el nombre para usarlo después
        rec_filename_ = filename;
        return true;
    }

    void AudioInputModule::uninit_rec_encoder() {
        ma_encoder_uninit(&pimpl_->encoder);
    }

    bool AudioInputModule::save_recording() {

        // Variable donde miniaudio guarda cuantos frames ha escrito realmente
        ma_uint64 framesWritten = 0;
        
        // Volcado a disco
        ma_result res = ma_encoder_write_pcm_frames(&pimpl_->encoder, rec_buffer_.data(), rec_buffer_.size() / channels_, &framesWritten);

        // Comprobar si se ha volcado bien
        if (res != MA_SUCCESS) {
            SYS_WARN("AudioInputModule","save_recording: ma_encoder_write_pcm_frames error");
            return false;
        }

        // Mostrar info del tiempo grabado y el archivo de salida
        float recTime_s = static_cast<float>(framesWritten) / static_cast<float>(sampleRate_);
        SYS_INFO("AudioInputModule","Record " + std::to_string(recTime_s) + "s to file '" + rec_filename_ + "'");
        rec_filename_.clear();

        // Limpiar el buffer de grabación
        rec_buffer_.clear();

        // Sale exitosamente
        return true;
    }


    // Suavizado de valores -----------------------------------------------------------------

    float AudioInputModule::smooth_level(
        float const     rawValue, 
        float const&    previousValue, 
        float           attackCoeff, 
        float           releaseCoeff)
    {
        // Si no se llama a esta función con los valores de ataque y release, se ponen los valores por defecto
        if (attackCoeff == 0)   attackCoeff  = attackCoeff_;
        if (releaseCoeff == 0)  releaseCoeff = releaseCoeff_;

        // Ahora, si los valores son realmente 0, devuelve el valor "crudo"
        if (attackCoeff == 0 || releaseCoeff == 0) return rawValue;

        // Cálculo de valor suavizado:
        float coeff = (rawValue > previousValue) ? attackCoeff : releaseCoeff;
        return coeff * rawValue + (1.0f - coeff) * previousValue;
    }



#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct AudioInputModule::Impl {};

// General ------------------------------------------------------------------------------
AudioInputModule::AudioInputModule(void*, void* const) : 
    max_int16_val_(std::numeric_limits<int16_t>::max())
{}
AudioInputModule::~AudioInputModule()   { return; }

// Ejecución -----------------------------------------------------------------------------
bool AudioInputModule::init(void*, std::string const&)  { return false; }
bool AudioInputModule::isInitialized() const            { return false; }
void AudioInputModule::loadConfig(void* config)         { return; }
void AudioInputModule::close()                          { return; }
bool AudioInputModule::reload()                         { return false; }
bool AudioInputModule::startCapture()                          { return false; }
bool AudioInputModule::stopCapture()                           { return false; }

// Información y parámetros --------------------------------------------------------------
std::string AudioInputModule::getDeviceName() const             { return ""; }
std::string AudioInputModule::getModuleName() const             { return ""; }
unsigned short AudioInputModule::getNumChannels() const         { return 0; }
unsigned short AudioInputModule::getSelectedChannel() const     { return 0; }        
unsigned int AudioInputModule::getSampleRate() const            { return 0; }
bool AudioInputModule::setDeviceName(std::string const&)        { return false; }
bool AudioInputModule::setNumChannels(unsigned short)           { return false; }
bool AudioInputModule::setSampleRate(unsigned int)              { return false; }
bool AudioInputModule::setSelectedChannel(unsigned short)       { return false; }
bool AudioInputModule::isValid() const                          { return false; }

// Captura ------------------------------------------------------------------------------
float  AudioInputModule::getRmsLevel()   const  { return 0.0f;  }
float  AudioInputModule::getPeakLevel()  const  { return 0;     }
size_t AudioInputModule::getBufferSize() const  { return 0;     }

// Callback expuesto --------------------------------------------------------------------
void AudioInputModule::setCallback_OnFrame(AudioCallback)    { return; }
void AudioInputModule::clearCallback_OnFrame()               { return; }

// Grabación ----------------------------------------------------------------------------
void AudioInputModule::StartRec(std::string const&) { return; }
void AudioInputModule::StopRec()                    { return; }
size_t AudioInputModule::getRecBufferSize()         { return 0; }
bool AudioInputModule::isRecording()                { return false; }

// Codificador de grabación (Privados) --------------------------------------------------
void AudioInputModule::init_rec_encoder(std::string const&) { return; }
void AudioInputModule::uninit_rec_encoder()                 { return; }
void AudioInputModule::save_recording()                    { return; }
    
// Suavizado de niveles -----------------------------------------------------------------
float AudioInputModule::smooth_level(float const,float const&,float,float) { return 0; }


#endif
