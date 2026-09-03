#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <limits>
    #include "system/SystemMgr.hpp"
    #include <files/JsonMgr.hpp>
    #include "sound/ACM_Imp.hpp"    // estructura PIMPL
    #include <algorithm>


    // General ------------------------------------------------------------------------------

    AudioCaptureModule::AudioCaptureModule(std::string const& moduleName, void* ctx) :
    pimpl_(std::make_unique<Impl>(ctx)),
    name_(moduleName),
    is_valid_(false),
    initialized_(false),
    running_(false),
    max_int16_val_(std::numeric_limits<int16_t>::max()),
    processBufferSize_(1024),
    smoothedValues_(false),
    attackCoeff_(0),
    releaseCoeff_(0)
    {

    }

    AudioCaptureModule::~AudioCaptureModule() {
        close();
    }


    // Inicialización -------------------------------------------------------------------

    bool AudioCaptureModule::init(void* config) {
        if (initialized_)
            return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

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

        // #TODO

    }

    bool AudioCaptureModule::close() {

        // Si no está inicializado, no hacer nada
        if (!initialized_)
            return false;

        // Info
        SYS_INFO("CaptureModule", "'" + name_ + "': Closing module, stopping all devices...");

        // Recorrer la lista de dispositivos
        for (auto& dev : pimpl_->devices) {
            if (!dev || !dev->initialized) continue;

            // Marco running false para que no salga warn de "input closed unexpectedly"
            running_ = false;

            SYS_INFO("CaptureModule", "'" + dev->alias + "': stopping device...");
            ma_device_stop(&dev->device);

            // Si estaba grabando, cerrar el encoder de forma segura, volcando lo que hubiera
            if (dev->recording_) {
                SYS_WARN("CaptureModule", "'" + dev->alias + "': device closed while recording, flushing partial file...");
                dev->recording_ = false;

                std::lock_guard<std::mutex> recLock(dev->rec_buffer_mtx_);
                if (dev->encoder_inited_) {
                    if (dev->channels > 0 && !dev->rec_buffer_.empty()) {
                        ma_uint64 framesWritten = 0;
                        ma_encoder_write_pcm_frames(&dev->encoder, dev->rec_buffer_.data(),
                            dev->rec_buffer_.size() / dev->channels, &framesWritten);
                    }
                    ma_encoder_uninit(&dev->encoder);
                    dev->encoder_inited_ = false;
                }
                dev->rec_buffer_.clear();
            }

            SYS_INFO("CaptureModule", "'" + dev->alias + "': uninit device...");
            ma_device_uninit(&dev->device);
            dev->initialized = false;
            dev->valid       = false;
        }

        // Limpieza de todos los dispositivos
        pimpl_->devices.clear();

        // Marcar como no inicializado y salir
        initialized_ = false;
        SYS_INFO("CaptureModule", "'" + name_ + "': Closed successfully.");
        return true;
    }

    bool AudioCaptureModule::reload() {

        SYS_INFO("CaptureModule", "Reloading module...");
        
        // Parar todo si está inicializado
        if (initialized_)
            close();

        // Inicializa tomando los parámetros nuevos para la config (samplerate, channels, etc.)
        return init();
    }


// Dispositivos del módulo --------------------------------------------------------------

    bool AudioCaptureModule::addCaptureDevice(
        std::string const&  deviceName, 
        unsigned int        channelSelected,
        unsigned int        sampleRate,
        std::string const&  deviceAlias) 
    {
        // Comprobar si hay contexto inicializado
        if (!pimpl_->ctx) {
            SYS_WARN("CaptureModule", "Cannot add new device: audio context not initialized");
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
        instance->sampleRate      = sampleRate;
        instance->owner           = this;

        // Inicializar el dispositivo de captura
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
        deviceConfig.capture.format       = ma_format_s16;
        deviceConfig.capture.channels     = 0;              // Inicializa con todos los canales disponibles
        deviceConfig.sampleRate           = sampleRate;
        deviceConfig.dataCallback         = Impl::dataCallback_;
        deviceConfig.notificationCallback = Impl::notificationCallback_;
        deviceConfig.pUserData            = instance.get();          // <- DeviceInstance*, no el módulo
        deviceConfig.capture.pDeviceID    = &instance->info.id;

        // Inicializar
        if (ma_device_init(pimpl_->ctx, &deviceConfig, &instance->device) != MA_SUCCESS) {
            SYS_WARN("CaptureModule", "'" + name_ + "': failed to init device " + instance->info.name);
            return false;
        }

        // Nº de canales REAL con el que se ha abierto el dispositivo (puede diferir del pedido)
        instance->channels = instance->device.capture.channels;

        // Validar el canal seleccionado contra los canales reales
        if (instance->selectedChannel > instance->channels) {
            SYS_WARN("CaptureModule","'" + name_ + "': selected_channel " + std::to_string(instance->selectedChannel)
                + " > " + std::to_string(instance->channels) + " canales disponibles. Se capturarán todos (0).");
            instance->selectedChannel = 0;
        }

        // Arrancar la captura de este dispositivo
        if (ma_device_start(&instance->device) != MA_SUCCESS) {
            SYS_WARN("CaptureModule", "'" + name_ + "': failed to start device " + instance->info.name);
            ma_device_uninit(&instance->device);
            return false;
        }

        instance->initialized = true;
        instance->valid       = true;
        instance->running     = true;

        SYS_INFO("CaptureModule","'" + name_ + "': added device '" + effectiveAlias + "' ("
            + selectedDeviceInfo->name + ", channel " + std::to_string(channelSelected)
            + ", " + std::to_string(instance->channels) + " real channels)");

        // Añadir a la lista de dispositicos
        pimpl_->devices.push_back(std::move(instance));

        // Comenzar a capturar audio por defecto
        running_ = startCapture(deviceAlias);

        return true;
    }

    bool AudioCaptureModule::removeCaptureDevice(std::string const& deviceAlias) {

        // Proteger la lista de dispositivos
        std::lock_guard<std::mutex> devicesLock(pimpl_->devices_mtx);

        // Buscar el dispositivo por alias
        auto it = std::find_if(pimpl_->devices.begin(), pimpl_->devices.end(),
            [&](std::unique_ptr<Impl::DeviceInstance> const& dev) {
                return dev->alias == deviceAlias;
            });
        if (it == pimpl_->devices.end()) {
            SYS_WARN("CaptureModule", "'" + name_ + "': removeCaptureDevice: alias '" + deviceAlias + "' not found");
            return false;
        }

        //  Obtener la instancia del dispositivo
        Impl::DeviceInstance* device = it->get();

        // Info
        SYS_INFO("CaptureModule", "'" + name_ + "': removing device '" + deviceAlias + "'...");

        // Marco false para que notificationCallback_ no dispare warn de "input closed unexpectedly"
        device->running = false;

        // Parar el dispositivo. 
        ma_device_stop(&device->device);

        // Si estaba grabando, parar y cerrar el encoder de forma segura
        if (device->recording_) {
            SYS_WARN("CaptureModule", "'" + deviceAlias + "': device removed while recording, flushing partial file...");
            device->recording_ = false;

            std::lock_guard<std::mutex> recLock(device->rec_buffer_mtx_);
            if (device->encoder_inited_) {
                if (device->channels > 0 && !device->rec_buffer_.empty()) {
                    ma_uint64 framesWritten = 0;
                    ma_encoder_write_pcm_frames(&device->encoder, device->rec_buffer_.data(),
                        device->rec_buffer_.size() / device->channels, &framesWritten);
                }
                ma_encoder_uninit(&device->encoder);
                device->encoder_inited_ = false;
            }
            device->rec_buffer_.clear();
        }

        // Eliminar de la lista
        pimpl_->devices.erase(it);

        // Info y salir
        SYS_INFO("CaptureModule", "'" + name_ + "': device '" + deviceAlias + "' removed");
        return true;
    }

    void AudioCaptureModule::setCallback_onDeviceResolve(std::function<const void*(std::string&)> cb) {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        onDeviceResolve_ = std::move(cb); 
    }

    void AudioCaptureModule::clearCallback_onDeviceResolve() {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        onDeviceResolve_ = nullptr;
    }

    bool AudioCaptureModule::hasCallback_onDeviceResolve() const {
        std::lock_guard<std::mutex> lk(onDeviceResolve_mtx_);
        return static_cast<bool>(onDeviceResolve_);
    }


    // Ejecución ----------------------------------------------------------------------------

    bool AudioCaptureModule::startCapture(std::string const& deviceAlias) {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("CaptureModule", "startCapture: device alias '" + deviceAlias + "' not found/initialized");
            return false;
        }
        if (device->running) return true;

        SYS_INFO("CaptureModule", "'" + deviceAlias + "': starting capture...");
        if (ma_device_start(&device->device) != MA_SUCCESS) {
            SYS_WARN("CaptureModule", "'" + deviceAlias + "': cannot start input device");
            return false;
        }
        device->running = true;
        return true;
    }

    bool AudioCaptureModule::stopCapture(std::string const& deviceAlias) {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("CaptureModule", "stopCapture: device alias '" + deviceAlias + "' not found/initialized");
            return false;
        }
        if (!device->running) return true;

        if (device->recording_) {
            SYS_WARN("CaptureModule", "'" + deviceAlias + "': device stopped while recording.");
            StopRec(deviceAlias);
        }

        device->running = false;   // ANTES de ma_device_stop, evita warning en notificationCallback_
        SYS_INFO("CaptureModule", "'" + deviceAlias + "': stopping capture...");
        return ma_device_stop(&device->device) == MA_SUCCESS;
    }


    // Parámetros del módulo ----------------------------------------------------------------

    std::string AudioCaptureModule::getModuleName() const {
        return name_;
    }

    unsigned int AudioCaptureModule::getSampleRate(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        return device ? device->sampleRate : 0;
    }

    bool AudioCaptureModule::isValid(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
            return device ? device->valid : false;
    };


    // Captura ------------------------------------------------------------------------------

    float AudioCaptureModule::getRmsLevel(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        return device ? device->rmsLevel.load() : 0.0f;
    }

    float AudioCaptureModule::getPeakLevel(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        return device ? device->peakLevel.load() : 0.0f;
    }

    size_t AudioCaptureModule::getBufferSize(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) return 0;
        std::lock_guard<std::mutex> lock(device->capture_buffer_mtx);
        return device->captureBuffer.size();
    }


    // Grabación ----------------------------------------------------------------------------

    bool AudioCaptureModule::StartRec(std::string const& deviceAlias, std::string const& filename) {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("CaptureModule", "StartRec: device alias '" + deviceAlias + "' not found/initialized");
            return false;
        }
        if (device->recording_) {
            SYS_WARN("CaptureModule", "'" + deviceAlias + "': already recording");
            return false;
        }
        device->recording_ = init_rec_encoder(filename, device);
        return device->recording_;
    }

    bool AudioCaptureModule::StopRec(std::string const& deviceAlias) {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) {
            SYS_WARN("CaptureModule", "StopRec: device alias '" + deviceAlias + "' not found/initialized");
            return false;
        }
        if (!device->recording_) return false;

        device->recording_ = false;
        bool res = save_recording(device);
        uninit_rec_encoder(device);
        return res;
    }

    size_t AudioCaptureModule::getRecBufferSize(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        if (!device) return 0;
        std::lock_guard<std::mutex> lock(device->rec_buffer_mtx_);
        return device->rec_buffer_.size();
    }

    bool AudioCaptureModule::isRecording(std::string const& deviceAlias) const {
        Impl::DeviceInstance* device = pimpl_->find_device(deviceAlias);
        return device ? device->recording_.load() : false;
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

    bool AudioCaptureModule::init_rec_encoder(std::string const& filename, void* devicePtr) {
        // Cast del dispositivo desde puntero opaco
        Impl::DeviceInstance* device = static_cast<Impl::DeviceInstance*>(devicePtr);

        // Le dices cómo quieres que sea el archivo.
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,
            ma_format_s16,
            device->channels,
            device->sampleRate
        );

        // Crea el archivo en el disco y prepara el encoder_ para escribir en él
        if (ma_encoder_init_file((filename + ".wav").c_str(), &config, &device->encoder) != MA_SUCCESS) {
            SYS_WARN("CaptureModule", "'" + device->alias + "': init_rec_encoder: ma_encoder_init_file error");
            return false;
        }

        device->encoder_inited_ = true;
        // Guarda el nombre para usarlo después
        device->rec_filename_   = filename;
        return true;
    }

    void AudioCaptureModule::uninit_rec_encoder(void* devicePtr) {
        // Cast del dispositivo desde puntero opaco
        Impl::DeviceInstance* device = static_cast<Impl::DeviceInstance*>(devicePtr);

        // Si está inicializado, desinicializar
        if (device->encoder_inited_) {
            ma_encoder_uninit(&device->encoder);
            device->encoder_inited_ = false;
        }
    }

    bool AudioCaptureModule::save_recording(void* devicePtr) {
        // Cast del dispositivo desde puntero opaco
        Impl::DeviceInstance* device = static_cast<Impl::DeviceInstance*>(devicePtr);

        // Protege el buffer de grabación
        std::lock_guard<std::mutex> lock(device->rec_buffer_mtx_);

        // Comprobar que el buffer tiene datos
        if (device->channels == 0 || device->rec_buffer_.empty()) {
            SYS_WARN("CaptureModule", "'" + device->alias + "': save_recording: nothing to write");
            return false;
        }

        // Variable donde miniaudio guarda cuantos frames ha escrito realmente
        ma_uint64 framesWritten = 0;
        
        // Volcado a disco
        if (ma_encoder_write_pcm_frames(&device->encoder, device->rec_buffer_.data(),
            device->rec_buffer_.size() / device->channels, &framesWritten) != MA_SUCCESS) 
        {
            SYS_WARN("CaptureModule","save_recording: ma_encoder_write_pcm_frames error");
            return false;
        }

        // Mostrar info del tiempo grabado y el archivo de salida
        float recTime_s = static_cast<float>(framesWritten) / static_cast<float>(device->sampleRate);
        SYS_INFO("CaptureModule", "'" + device->alias + "': Record " + std::to_string(recTime_s) + "s to file '" + device->rec_filename_ + "'");
        device->rec_filename_.clear();

        // Limpiar el buffer de grabación
        device->rec_buffer_.clear();

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
