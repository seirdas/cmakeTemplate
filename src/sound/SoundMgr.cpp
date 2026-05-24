#include "sound/SoundMgr.hpp"

// Macro de cmake al activar la librería
#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "sound/AudioInputModule.hpp"
    #include "sound/AudioPlaybackModule.hpp"
    #include <memory>
    #include <chrono>
    #include <thread>
    #include "system/SystemMgr.hpp"

    // Implementación de miembros de la clase de miniaudio (pimpl_)
    struct SoundMgr::Impl {
        ma_context snd_context_;                            // Contexto (motor) de audio

        ma_device_info* pPlaybackDevInfos_   = nullptr;     // Información de dispositivos playback
        ma_uint32       PlaybackDevCount_    = 0;           // Número de dispositivos playback
        ma_device_info* pCaptureDevInfos_ = nullptr;        // Información de dispositivos de captura
        ma_uint32       captureDevCount_  = 0;              // Número de dispositivos de captura
    };


    // General ------------------------------------------------------------------------------

    SoundMgr::SoundMgr() : 
        pimpl_(std::make_unique<Impl>()),
        ctx_initialized_(false)
    {

    }

    SoundMgr::~SoundMgr() {
        stop();
    }

    bool SoundMgr::init() {

        // No hacer nada si ya se ha iniciado
        if (ctx_initialized_) return true;
        SYS_INFO("SoundMgr", "Initializating sound context...");

        // Inicializar sistema de audio
        ma_result res = ma_context_init(NULL, 0, NULL, &pimpl_->snd_context_);
        ctx_initialized_ = (res == MA_SUCCESS) ? true : false;
        if (!ctx_initialized_) {
            SYS_ERROR("SoundMgr","Cannot initialize audio system (ma_context_init).");
            return false;
        }

        // Obtener dispositivos de captura/playback
        if (!updateDevices())
            SYS_WARN("SoundMgr","Failed to get playback devices.");

        return ctx_initialized_;
    }

    bool SoundMgr::stop() {
        // No hacer nada si ya se ha cerrado.
        if (!ctx_initialized_) return true;
        SYS_INFO("SoundMgr", "Closing sound engine and modules...");

        // Limpieza (destruir) los módulos creados
        inputs_.clear();
        playbacks_.clear();

        // Desinicializar el contexto global
        ma_context_uninit(&pimpl_->snd_context_);
        ctx_initialized_ = false;

        // Notificar y salir
        SYS_INFO("SoundMgr", "Sound system stopped successfully.");
        return true;
    }

    bool SoundMgr::updateDevices() {
        // Comprobar que el contexto está inicializado
        if (!ctx_initialized_) return false;

        // Obtener los dispositivos de reproducción y captura
        ma_result res = ma_context_get_devices(&pimpl_->snd_context_,
            &pimpl_->pPlaybackDevInfos_, &pimpl_->PlaybackDevCount_, 
            &pimpl_->pCaptureDevInfos_, &pimpl_->captureDevCount_);

        // Devolver true/false en función del resultado
        return (res==MA_SUCCESS) ? true : false;
    }


    // Capture Input ------------------------------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailableInputs() const {
        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        // Popular vector con dispositivos disponibles
        std::vector<std::string> devlist;
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) 
            devlist.push_back(pimpl_->pCaptureDevInfos_[i].name);

        // Devolver la lista de dispositivos
        return devlist;
    }

    std::string SoundMgr::getDefaultInputDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        // Popular vector con dispositivos disponibles
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
            if (pimpl_->pCaptureDevInfos_[i].isDefault)
                return pimpl_->pCaptureDevInfos_[i].name;

        /*else*/ return "";
    }
    
    void SoundMgr::listAvailableInputs() {

        // Primero actualizar la lista dse dispositivos disponibles
        updateDevices();

        SYS_INFO("SoundMgr","--- CAPTURE DEVICES ---");
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) {
            SYS_INFO(
                "SoundMgr",
                "[" + std::to_string(i) + "] " 
                + pimpl_->pCaptureDevInfos_[i].name
                + ( (pimpl_->pCaptureDevInfos_[i].isDefault) ? " (Default)" : "" )
            );
        }
    }

    bool SoundMgr::addCaptureDevice(std::string const& name, unsigned short index){
        // #TODO
        return false;
    }


    // Playbacks ----------------------------------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const {
        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        std::vector<std::string> devlist;
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i) 
            devlist.push_back(pimpl_->pPlaybackDevInfos_[i].name);
        return devlist;
    }

    std::string SoundMgr::getDefaultPlaybackDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        // Popular vector con dispositivos disponibles
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i)
            if (pimpl_->pPlaybackDevInfos_[i].isDefault)
                return pimpl_->pPlaybackDevInfos_[i].name;

        /*else*/ return "";
    }

    void SoundMgr::listAvailablePlaybacks() {
        
        // Primero actualizar la lista dse dispositivos disponibles
        updateDevices();

        SYS_INFO("SoundMgr","--- PLAYBACK DEVICES ---");
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i) {
            SYS_INFO(
                "SoundMgr",
                "[" + std::to_string(i) + "] " 
                + pimpl_->pPlaybackDevInfos_[i].name
                + ( (pimpl_->pPlaybackDevInfos_[i].isDefault) ? " (Default)" : "" )
            );
        }
    }

    bool SoundMgr::addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder) {
        // Comprobar que el contexto está inicializado
        if (!ctx_initialized_) {
            SYS_WARN("SoundMgr","Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // bucle para encontrar el ma_device_info por el nombre
        ma_device_info* selectedDeviceInfo = nullptr;
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i) {
            if (deviceName == pimpl_->pPlaybackDevInfos_[i].name) {
                selectedDeviceInfo = &pimpl_->pPlaybackDevInfos_[i];
                break;
            }
        }

        // Si no ha encontrado nada saltar fallo y return
        if (selectedDeviceInfo == nullptr) {
            SYS_WARN("SoundMgr","Failed to found device with name");
            return false;
        }
        SYS_INFO("SoundMgr", "Using playback device: " + deviceName);

        // Crear receiver (aún no registrado) #TODO AÑADIR AudioFilesFolder
        std::unique_ptr<AudioPlaybackModule> apm = std::make_unique<AudioPlaybackModule>(
            &pimpl_->snd_context_, 
            *selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback...");
        if (!apm->start())
        {
            // No hay nada que limpiar, el puntero make_unique se destruye al salir.
            SYS_WARN("SoundMgr","Failed to initialize playback.");
            return false;
        }

        // Insertar en el vector
        playbacks_.push_back(std::move(apm));
        SYS_INFO("SoundMgr", "Playback loaded. ");

        return true;
    }

    bool SoundMgr::removePlaybackDevice(unsigned short index) {

        // comprobar si existe
        if (index >= playbacks_.size()) {
            SYS_WARN("SoundMgr","Selected index " + std::to_string(index) +
                " out of bounds (" + std::to_string(playbacks_.size()) + ")");;
            return false;
        }
        
        // Obtener el módulo de reproducción
        AudioPlaybackModule* apm = playbacks_[index].get();

        // Detener las operaciones del módulo de reproducción
        apm->stop();

        // Borrar el elemento del vector usando iterator
        playbacks_.erase(playbacks_.begin() + index);

        // Notificar y salir
        SYS_INFO("SoundMgr", "Deleted Playback.");
        return true;
    }

    bool SoundMgr::playbackTest() {

        if (!ctx_initialized_) {
            SYS_WARN("SoundMgr","Audio context not initialized.");
            return false;
        }

        // Actualizar dispositivos disponibles
        updateDevices();

        // Tomar el PRIMER dispositivo de audio
        std::vector<std::string> list = getAvailablePlaybacks();
        if (list.empty())
            return false;
        //addPlaybackDevice(list[0], "audio");        // <-- Aquí se hace también start()

        // Voy a probar tomando el dispositivo de audio predeterminado
        addPlaybackDevice(getDefaultPlaybackDevice(), "audioTest");

        // puntero al APM que acabamos de meter
        if (playbacks_.empty()) 
            return false;
        AudioPlaybackModule* ultimoAPM = playbacks_.back().get();
        SYS_INFO("SoundMgr", "Testing device: " + ultimoAPM->deviceName());

        /* precarga opcional */
        ultimoAPM->preload("audio/DefaultDance.mp3");
        ultimoAPM->preload("audio/ding.mp3");

        /* reproducir */
        SoundID ding = ultimoAPM->play(
            "audio/ding.mp3",
            1.0f,
            1.0f,
            LoopMode::LOOP);

        SoundID click = ultimoAPM->play("audio/DefaultDance.mp3");

        SYS_INFO("SoundMgr", "Sleep for 500ms...");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
        
        /* modificar mientras reproduce */
        ultimoAPM->setVolume(ding, 0.4f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
        ultimoAPM->setVolume(click, 0.3f);
        ultimoAPM->setPitch(click, 1.2f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
        
        /* cortar música */
        ultimoAPM->stopSound(ding);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

        // Opcional: limpiar el módulo (destruir sonidos)
        ultimoAPM->stop();

        // Remover el APM de la lista
        removePlaybackDevice(static_cast<unsigned short>(playbacks_.size() - 1));

        return true;
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    SoundMgr() {}
    ~SoundMgr() {}
    bool SoundMgr::init()            { return false; }
    bool SoundMgr::stop();           { return false; }
    bool SoundMgr::updateDevices();  { return false; }

    // Capture Input ------------------------------------------------------------------------
    std::vector<std::string> SoundMgr::getAvailableInputs() const;                         { return {}; }
    std::string SoundMgr::getDefaultInputDevice() const                                    { return {}; }
    void        SoundMgr::listAvailableInputs() const                                      { return; }
    bool        SoundMgr::addCaptureDevice(std::string const& name, unsigned short index); { return false; }

    // Playbacks ----------------------------------------------------------------------------
    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const;   { return {}; }
    std::string SoundMgr::getDefaultPlaybackDevice() const              { return {}; }
    void        SoundMgr::listAvailablePlaybacks() const                { return; }
    bool        SoundMgr::addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder) { return false; }
    bool        SoundMgr::removePlaybackDevice(unsigned short index)    { return false; }
    bool        SoundMgr::playbackTest()                                { return false; }

#endif
