#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <limits>
    #include "system/SystemMgr.hpp"
    #include <files/JsonMgr.hpp>
    #include "sound/ACM_Imp.hpp"    // estructura PIMPL


    // General ------------------------------------------------------------------------------

    AudioCaptureModule::AudioCaptureModule(std::string const& moduleName, void* ctx, const void* device_info) :
    pimpl_(std::make_unique<Impl>(ctx, device_info)),
    name_(moduleName),
    is_valid_(false),
    initialized_(false),
    running_(false),
    channels_(0),
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

    AudioCaptureModule::~AudioCaptureModule() {
        close();
    }


    // Inicialización -------------------------------------------------------------------

    bool AudioCaptureModule::init(void* config, std::string const& captureName) {
        if (initialized_)
            return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

        // Ponerle nombre a este módulo (sobreescribe el de la config)
        if (!captureName.empty()) name_ = captureName;

        // Inicializar el dispositivo de captura
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

        // rellenar los parámetros de la configuración de miniaudio
        // channels_ == 0 -> miniaudio abre el dispositivo con sus canales nativos
        deviceConfig.capture.format       = ma_format_s16;
        deviceConfig.capture.channels     = static_cast<unsigned int>(channels_);
        deviceConfig.sampleRate           = sampleRate_;
        deviceConfig.dataCallback         = Impl::dataCallback_;
        deviceConfig.notificationCallback = Impl::notificationCallback_;
        deviceConfig.pUserData            = this;
        deviceConfig.capture.pDeviceID    = &pimpl_->device_info.id;

        // Inicializar
        if (ma_device_init(pimpl_->ctx, &deviceConfig, &pimpl_->device) != MA_SUCCESS) {
            SYS_WARN("CaptureModule","Cannot initialize input device");
            is_valid_ = false;
            return false;
        }

        // Nº de canales REAL con el que se ha abierto el dispositivo (puede diferir del pedido)
        channels_ = static_cast<unsigned short>(pimpl_->device.capture.channels);

        // Validar el canal seleccionado contra los canales reales (0 = todos, 1..N = ese canal)
        if (selectedChannel_ > channels_) {
            SYS_WARN("CaptureModule","'" + name_ + "': selected_channel " + std::to_string(selectedChannel_)
                + " > " + std::to_string(channels_) + " canales disponibles. Se capturarán todos (0).");
            selectedChannel_ = 0;
        }

        SYS_INFO("CaptureModule","'" + name_ + "': " + std::to_string(channels_)
            + " canal(es), seleccionado: " + std::to_string(selectedChannel_));

        // Comenzar a capturar audio
        running_ = startCapture();

        // Llega hasta aquí si se ha inicializado bien
        initialized_ = true;
        return initialized_; //<- true
    }

    bool AudioCaptureModule::isInitialized() const {
        return initialized_;
    }
    
    void AudioCaptureModule::loadConfig(void* config) {
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

    void AudioCaptureModule::close() {

        // Si no está inicializado, no hacer nada
        if (!initialized_)
            return;

        // Marco running false para que no salga warn de "input closed unexpectedly"
        running_ = false;

        // Parar la captura
        SYS_INFO("CaptureModule","'" + name_ + "': Stopping device...");
        stopCapture();

        // Desinicializa el dispositivo
        SYS_INFO("CaptureModule","'" + name_ + "': Uninit device...");
        ma_device_uninit(&pimpl_->device);
        is_valid_ = false;

        // Limpia el callback
        SYS_INFO("CaptureModule","'" + name_ + "': Clearing onframe callback injected...");
        clearCallback_OnFrame();

        initialized_ = false;
    }

    bool AudioCaptureModule::reload() {

        SYS_INFO("CaptureModule", "Reloading module...");
        
        // Parar todo si está inicializado
        if (initialized_)
            close();

        // Inicializa tomando los parámetros nuevos para la config (samplerate, channels, etc.)
        return init();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool AudioCaptureModule::startCapture() {
        if (running_)
            return true;

        SYS_INFO("CaptureModule","Initializing capture...");
        if (ma_device_start(&pimpl_->device) != MA_SUCCESS){
            SYS_WARN("CaptureModule","Cannot start input device");
            ma_device_uninit(&pimpl_->device);
            return false;
        }
        else return true;
    }

    bool AudioCaptureModule::stopCapture() {
        if (!running_)
            return true;

        // Si estaba grabando, parar la grabación (se guarda lo que hubiera)
        if (recording_) {
            SYS_WARN("CaptureModule","Device stopped while recording.");
            StopRec();
            rec_buffer_.clear();
        }

        // Detener la captura de audio
        SYS_INFO("CaptureModule","Stopping device...");
        ma_result res = ma_device_stop(&pimpl_->device);
        return res == MA_SUCCESS;
    }


    // Parámetros del módulo ----------------------------------------------------------------

    std::string AudioCaptureModule::getDeviceName() const { 
        return pimpl_->device_info.name; 
    }

    std::string AudioCaptureModule::getModuleName() const {
        return name_;
    }

    unsigned short AudioCaptureModule::getNumChannels() const {
        return channels_;
    }

    unsigned short AudioCaptureModule::getSelectedChannel() const {
        return selectedChannel_;
    }

    unsigned int AudioCaptureModule::getSampleRate() const {
        return sampleRate_;
    }

    bool AudioCaptureModule::setDeviceName(std::string const& deviceName) {

        // Obtener el ma_device_info a partir del nombre
        ma_device_info* selectedDeviceInfo  = nullptr;
        unsigned int    captureDevCount     = 0;
        ma_device_info* pCaptureDevInfos   = nullptr;

        // Obtener los dispositivos de captura (simplificado)
        ma_result res = ma_context_get_devices(pimpl_->ctx,
            nullptr, nullptr, 
            &pCaptureDevInfos, &captureDevCount);

        // Comprobar si se han obtenido bien (si no, continúa igual...)
        if (res != MA_SUCCESS)
            SYS_WARN("CaptureModule","Failed retrieving available captures");

        // Obtener el dispositivo a partir del nombre (No hace comprobación de duplicados ni inferencia de nombre)
        for (unsigned int i = 0; i < captureDevCount; ++i)
            if (deviceName == pCaptureDevInfos[i].name) {  
                selectedDeviceInfo = &pCaptureDevInfos[i];
                break;
            }

        // Si no encuentra ningún nombre salta fallo
        if(!selectedDeviceInfo){
            SYS_WARN("CaptureModule", "Failed to found device: '" + deviceName + "'"); 
            return false; 
        }

        // Guarda el dispositivo encontrado
        pimpl_->device_info = *selectedDeviceInfo;

        // Que el nuevo dispositivo se abra con sus canales nativos (init lee el real tras abrir)
        channels_ = 0;

        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioCaptureModule::setNumChannels(unsigned short numChannels) {
        channels_ = numChannels;
        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioCaptureModule::setSampleRate(unsigned int sampleRate) {
        sampleRate_ = sampleRate;
        // Reinicializa (si aplica)
        return initialized_ ? reload() : true;
    }

    bool AudioCaptureModule::setSelectedChannel(unsigned short selectedChannel) {
        if (selectedChannel > channels_) {
            SYS_WARN("CaptureModule","Cannot change channel: selected:" 
                + std::to_string(selectedChannel) + ", channels: " + std::to_string(channels_));
                return false;
        }
        selectedChannel_ = selectedChannel;
        return true;
    }

    bool AudioCaptureModule::isValid() const {
        return is_valid_;
    };


    // Captura ------------------------------------------------------------------------------

    float AudioCaptureModule::getRmsLevel() const { 
        return rmsLevel_; 
    }

    float AudioCaptureModule::getPeakLevel() const { 
        return peakLevel_; 
    }
    
    size_t AudioCaptureModule::getBufferSize() const {
        return captureBuffer_.size();
    };


    // Callback expuesto --------------------------------------------------------------------
    
    void AudioCaptureModule::setCallback_OnFrame(AudioCallback cb) {
        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_cb_ = std::move(cb); 
    }

    void AudioCaptureModule::clearCallback_OnFrame() {
        if (!onFrame_cb_) return;
        std::lock_guard<std::mutex> lk(onFrame_mtx_);
        onFrame_cb_ = nullptr;
    }

    bool AudioCaptureModule::hasCallback_OnFrame() {
        return static_cast<bool>(onFrame_cb_);
    }


    // Grabación ----------------------------------------------------------------------------

    bool AudioCaptureModule::StartRec(std::string const& filename) {
        // Inicializar el encoder (necesita el nombre de archivo) con 
        // flag para activar la toma de samples en el buffer de grabación del callback de captura
        recording_ = init_rec_encoder(filename);
        return recording_;
    }

    bool AudioCaptureModule::StopRec() {

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

    size_t AudioCaptureModule::getRecBufferSize() const {
        return rec_buffer_.size();
    };

    bool AudioCaptureModule::isRecording() const {
        return recording_; 
    }


    // Parámetros de suavizado de valores ---------------------------------------------------

    void AudioCaptureModule::enableSmoothedValues(bool value) {
        smoothedValues_ = value;
    }

    void AudioCaptureModule::setSmoothAttackCoeff(float value) {
        attackCoeff_ = value;
    }

    void AudioCaptureModule::setSmoothReleaseCoeff(float value) {
        releaseCoeff_ = value;
    }


    // Codificador de grabación -------------------------------------------------------------

    bool AudioCaptureModule::init_rec_encoder(std::string const& filename) {

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
            SYS_WARN("CaptureModule","init_rec_encoder: ma_encoder_init_file error");
            return false;
        }

        // Guarda el nombre para usarlo después
        rec_filename_ = filename;
        return true;
    }

    void AudioCaptureModule::uninit_rec_encoder() {
        ma_encoder_uninit(&pimpl_->encoder);
    }

    bool AudioCaptureModule::save_recording() {

        // Variable donde miniaudio guarda cuantos frames ha escrito realmente
        ma_uint64 framesWritten = 0;
        
        // Volcado a disco
        ma_result res = ma_encoder_write_pcm_frames(&pimpl_->encoder, rec_buffer_.data(), rec_buffer_.size() / channels_, &framesWritten);

        // Comprobar si se ha volcado bien
        if (res != MA_SUCCESS) {
            SYS_WARN("CaptureModule","save_recording: ma_encoder_write_pcm_frames error");
            return false;
        }

        // Mostrar info del tiempo grabado y el archivo de salida
        float recTime_s = static_cast<float>(framesWritten) / static_cast<float>(sampleRate_);
        SYS_INFO("CaptureModule","Record " + std::to_string(recTime_s) + "s to file '" + rec_filename_ + "'");
        rec_filename_.clear();

        // Limpiar el buffer de grabación
        rec_buffer_.clear();

        // Sale exitosamente
        return true;
    }


    // Suavizado de valores -----------------------------------------------------------------

    float AudioCaptureModule::smooth_level(
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

    struct AudioCaptureModule::Impl {};

    // General ------------------------------------------------------------------------------
    AudioCaptureModule::AudioCaptureModule(std::string const& moduleName, void* ctx, const void* device_info) : 
        pimpl_(std::make_unique<Impl>()), 
        name_(moduleName), 
        is_valid_(false), 
        initialized_(false), 
        running_(false), 
        channels_(0), 
        sampleRate_(0), 
        deviceName_(""), 
        selectedChannel_(0), 
        codec_inited_(false), 
        recording_(false), 
        rec_filename_(""), 
        max_int16_val_(32767), 
        rmsLevel_(0.0f), 
        peakLevel_(0.0f), 
        processBufferSize_(0), 
        smoothedValues_(false), 
        attackCoeff_(0.0f), 
        releaseCoeff_(0.0f)                                                                 { }

    AudioCaptureModule::~AudioCaptureModule()                                               { }

    // Inicialización -----------------------------------------------------------------------
    bool AudioCaptureModule::init(void*, std::string const&)                                { return false; }
    bool AudioCaptureModule::isInitialized() const                                          { return false; }
    void AudioCaptureModule::loadConfig(void*)                                              { }
    void AudioCaptureModule::close()                                                        { }
    bool AudioCaptureModule::reload()                                                       { return false; }

    // Ejecución ----------------------------------------------------------------------------
    bool AudioCaptureModule::startCapture()                                                 { return false; }
    bool AudioCaptureModule::stopCapture()                                                  { return false; }

    // Parámetros del módulo ----------------------------------------------------------------
    std::string AudioCaptureModule::getDeviceName() const                                   { return ""; }
    std::string AudioCaptureModule::getModuleName() const                                   { return ""; }
    unsigned short AudioCaptureModule::getNumChannels() const                               { return 0; }
    unsigned short AudioCaptureModule::getSelectedChannel() const                           { return 0; }
    unsigned int AudioCaptureModule::getSampleRate() const                                  { return 0; }
    bool AudioCaptureModule::setDeviceName(std::string const&)                              { return false; }
    bool AudioCaptureModule::setNumChannels(unsigned short)                                 { return false; }
    bool AudioCaptureModule::setSampleRate(unsigned int)                                    { return false; }
    bool AudioCaptureModule::setSelectedChannel(unsigned short)                             { return false; }
    bool AudioCaptureModule::isValid() const                                                { return false; }

    // Captura ------------------------------------------------------------------------------
    float AudioCaptureModule::getRmsLevel() const                                           { return 0.0f; }
    float AudioCaptureModule::getPeakLevel() const                                          { return 0.0f; }
    size_t AudioCaptureModule::getBufferSize() const                                        { return 0; }

    // Callback expuesto --------------------------------------------------------------------
    void AudioCaptureModule::setCallback_OnFrame(AudioCallback)                             { }
    void AudioCaptureModule::clearCallback_OnFrame()                                        { }
    bool AudioCaptureModule::hasCallback_OnFrame()                                          { return false; }

    // Grabación ----------------------------------------------------------------------------
    bool AudioCaptureModule::StartRec(std::string const&)                                   { return false; }
    bool AudioCaptureModule::StopRec()                                                      { return false; }
    size_t AudioCaptureModule::getRecBufferSize() const                                     { return 0; }
    bool AudioCaptureModule::isRecording() const                                            { return false; }

    // Parámetros de suavizado de valores ---------------------------------------------------
    void AudioCaptureModule::enableSmoothedValues(bool)                                     { }
    void AudioCaptureModule::setSmoothAttackCoeff(float)                                    { }
    void AudioCaptureModule::setSmoothReleaseCoeff(float)                                   { }

    // Privados -----------------------------------------------------------------------------
    bool AudioCaptureModule::init_rec_encoder(std::string const&)                           { return false; }
    void AudioCaptureModule::uninit_rec_encoder()                                           { }
    bool AudioCaptureModule::save_recording()                                               { return false; }
    float AudioCaptureModule::smooth_level(float const, float const&, float, float)         { return 0.0f; }

#endif
