#include "sound/SoundMgr.hpp"
#include <algorithm>

// Macro de cmake al activar la librería
#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <memory>
    #include <chrono>
    #include <thread>
    #include "sound/AudioInputModule.hpp"
    #include "sound/AudioPlaybackModule.hpp"
    #include "system/SystemMgr.hpp"
    #include "files/JsonMgr.hpp"    // Para conocer json


    // Implementación de miembros de la clase de miniaudio (pimpl_)
    struct SoundMgr::Impl {
        ma_context      snd_context_;                       ///< Contexto (motor) de audio
        ma_device_info* pPlaybackDevInfos_  = nullptr;      ///< Información de dispositivos playback
        ma_uint32       PlaybackDevCount_   = 0;            ///< Número de dispositivos playback
        ma_device_info* pCaptureDevInfos_   = nullptr;      ///< Información de dispositivos de captura
        ma_uint32       captureDevCount_    = 0;            ///< Número de dispositivos de captura
    };

    // General ------------------------------------------------------------------------------

    SoundMgr::SoundMgr() : 
        pimpl_(std::make_unique<Impl>()),
        initialized_(false),
        MAX_REINIT_ATTEMPTS(3),
        smoothedValues_(true),
        attackCoeff_(0.5),
        releaseCoeff_(0.1)
    {

    }

    SoundMgr::~SoundMgr() {
        stop();
    }

    bool SoundMgr::init(void* config) {

        // No hacer nada si ya se ha iniciado
        if (initialized_) return true;
        SYS_INFO("SoundMgr", "Initializating sound context...");

        // Inicializar sistema de audio
        ma_result res = ma_context_init(NULL, 0, NULL, &pimpl_->snd_context_);
        initialized_ = (res == MA_SUCCESS) ? true : false;
        if (!initialized_) {
            SYS_ERROR("SoundMgr","Cannot initialize audio system (ma_context_init).");
            return false;
        }

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else
            SYS_WARN("SoundMgr","Cannot load config. Using default values.");

        // Obtener dispositivos de captura/playback
        if (!updateDevices())
            SYS_WARN("SoundMgr","Failed to get playback devices.");

        return initialized_;
    }

    bool SoundMgr::isInitialized() const {
        return initialized_;
    }

    void SoundMgr::loadConfig(void* config) {
        if (!config) 
            return;
            
        // Se considera que la configuración se pasa como json    
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Valores globales de suavizado (para los módulos capture/playbacks)
        float minValue = 0; float maxValue = 1;
        jsonMgr.get_or_set(cfg, "smoothedValues",       smoothedValues_);
        jsonMgr.get_or_set(cfg, "attack_coefficient",   attackCoeff_);
        attackCoeff_= std::clamp(attackCoeff_, minValue, maxValue);
        jsonMgr.get_or_set(cfg, "release_coefficient",  releaseCoeff_);
        releaseCoeff_= std::clamp(releaseCoeff_, minValue, maxValue);

        /* CAPTURAS */
        // Recorrer elementos de capturas dentro del nodo json
        std::vector<json*> config_elements = jsonMgr.getArrayElements(cfg, "Capture");
        for (short i=0; i < config_elements.size(); i++) {
            SYS_INFO("SoundMgr","Loading capture from config...");
            addCaptureDevice(config_elements[i]);
        }

        /* PLAYBACKS */
        // #TODO
    }

    bool SoundMgr::stop() {
        // No hacer nada si ya se ha cerrado.
        if (!initialized_) return true;
        SYS_INFO("SoundMgr", "Closing sound engine and modules...");

        // Limpieza (destruir) los módulos creados
        captures_.clear();
        playbacks_.clear();

        // Desinicializar el contexto global
        ma_context_uninit(&pimpl_->snd_context_);
        initialized_ = false;

        // Notificar y salir
        SYS_INFO("SoundMgr", "Sound system stopped successfully.");
        return true;
    }

    bool SoundMgr::updateDevices() {
        // Comprobar que el contexto está inicializado
        if (!initialized_) return false;

        // Obtener los dispositivos de reproducción y captura
        ma_result res = ma_context_get_devices(&pimpl_->snd_context_,
            &pimpl_->pPlaybackDevInfos_, &pimpl_->PlaybackDevCount_, 
            &pimpl_->pCaptureDevInfos_, &pimpl_->captureDevCount_);

        if (res != MA_SUCCESS) return false;

        // Si hay algun dispositivo con is_valid = false se reinicializa
        for (auto& [name, aim] : captures_) {
            if (aim->isValid()) continue;

            for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) 
                if (aim->deviceName() == pimpl_->pCaptureDevInfos_[i].name) 
                    for (unsigned int tries = 0; tries < MAX_REINIT_ATTEMPTS; tries++) 
                        if (aim->init(nullptr))
                            break;
        }

        return true;
    }


    // Dispositivos de captura --------------------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailableInputs() const {

        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Crea un vector de strings devlist
        std::vector<std::string> devlist;
        // Recorre los dispositivos de captura que miniaudio encuentra
        // Se guardan en captureDevCount
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) 
            devlist.push_back(pimpl_->pCaptureDevInfos_[i].name);

        // Devolver la lista de dispositivos
        return devlist;
    }

    std::vector<std::string> SoundMgr::getManagedCaptures() const {
        std::vector<std::string> cap;
        for (auto& it : captures_)
            cap.push_back(it.first);
        return cap;
    }
    
    bool SoundMgr::isOnManagedCaptures(std::string const& captureName) const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Recorre todos los dispositivos de captura 
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
            if (pimpl_->pCaptureDevInfos_[i].name == captureName)
                return true;

        /*else*/ return false;
    }

    std::string SoundMgr::getDefaultInputDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Recorre todos los dispositivos de captura 
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
        // Cuando encuentra el que tiene Default = true devuelve su nombre
            if (pimpl_->pCaptureDevInfos_[i].isDefault)
                return pimpl_->pCaptureDevInfos_[i].name;

                // Si no hay nungún predeterminado, devuelve un ""
        /*else*/ return "";
    }
    
    bool SoundMgr::addCaptureDevice(void* config, std::string const& captureName, std::string const& deviceName) {
     
        // Comprobar que el contexto está inicializado
        if (!initialized_){
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false; 
        }

        // Intentar obtener nombre y dispositivo del param o de la config
        std::string myCaptureName = captureName;
        std::string myDeviceName = deviceName;
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (myCaptureName.empty() && myDeviceName.empty() ) {
            if (!config) {
                SYS_WARN("SoundMgr","addCaptureDevice: Cannot retrieve capture device name.");
                return false;
            }
            // Intentar obtener del json
            if (myCaptureName.empty())
                jsonMgr.get(static_cast<json*>(config), "device", myDeviceName);
            if (myCaptureName.empty())
                jsonMgr.get(static_cast<json*>(config), "name", myCaptureName);
        }

        // Comprobación del dispositivo
        if (myDeviceName.empty()) {
            SYS_WARN("SoundMgr","addCaptureDevice: Cannot retrieve device name.");
            return false;
        }
        // Comprobación del nombre: Le ponemos un nombre random si no tiene
        if (myCaptureName.empty()) {
            myCaptureName = "CAPTURE#" + std::to_string(rand());
        }

        // Refrescar lista de dispositivos disponibles
        updateDevices();

        // BUGFIX: Si encuentra un dispositivo con el nombre entero literal, no busca si contiene el nombre                             
        ma_device_info* selectedDeviceInfo = nullptr;
        bool found = false;
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
            if (myDeviceName == pimpl_->pCaptureDevInfos_[i].name) {  
                selectedDeviceInfo = &pimpl_->pCaptureDevInfos_[i];
                found = true;
                break;
            }
        if (!found) {
            // bucle para encontrar el dispositivo cuyo nombre real contenga el deviceName
            short count = 0;
            std::string realName = "";           
            for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) { 
                realName = pimpl_->pCaptureDevInfos_[i].name;          
                if (realName.find(myDeviceName) != std::string::npos) {  
                    selectedDeviceInfo = &pimpl_->pCaptureDevInfos_[i];
                    count++;
                }
            }

            // Sale si se han encontrado varios con el mismo nombre
            if (count > 1) {
                SYS_WARN("SoundMgr","Cannot initialize audio input: Ambiguous name specified: '" + myDeviceName + "'");
                return false;
            }
        }

        // Si no encuentra ningún nombre salta fallo
        if(!selectedDeviceInfo){
            SYS_WARN("SoundMgr", "Failed to found device: '" + myDeviceName + "'"); 
            return false; 
        }

        // corrige la config con el nombre completo real para que quede guardado igual
        myDeviceName = selectedDeviceInfo->name; 
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", myDeviceName);
            
        SYS_INFO("SoundMgr", "Initializing capture device: " + myDeviceName); 

        // Crear AudioInputModule
        std::unique_ptr<AudioInputModule> aim = std::make_unique<AudioInputModule>(
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intenta inicializar el AudioInputModule
        if(!aim->init(config))
        {
            SYS_WARN("SoundMgr","Failed to initialize capture.");
            return false;
        }

        // Le pasa los parámetros globales del suavizado de valores
        aim->set_SmoothedValues(smoothedValues_);
        aim->set_SmoothAttackCoeff(attackCoeff_);
        aim->set_SmoothReleaseCoeff(releaseCoeff_);

        // El micrófono que acabas de crear (aim) lo metes en la lista de micrófonos (captures_).
        captures_[myCaptureName] = std::move(aim);
        SYS_INFO("SoundMgr", "New input device: '" + myCaptureName + "' (" + myDeviceName + ")");

        return true;
    }

    bool SoundMgr::removeInputDevice(std::string const& name) {

        // comprobar si existe
        auto it = captures_.find(name);
        if (it == captures_.end()) {
            SYS_WARN("SoundMgr", "Selected device '" + name + "' not found.");
            return false;
        }

        // Detener las operaciones del módulo de reproducción
        it->second->stop();

        // Borrar el elemento del vector usando iterator
        captures_.erase(it);

        // Notificar y salir
        SYS_INFO("SoundMgr", "Deleted Capture Device.");
        return true;
    }

    // Ejecución y datos en dispositivos de captura -----------------------------------------

    bool SoundMgr::startRec(std::string const& name){
        auto it = captures_.find(name);
        if (it == captures_.end()) return false;

        std::string filename = "grabacion_" + name;
        it->second->StartRec(filename);

        return true;
    }

    bool SoundMgr::stopRec(std::string const& name){
        auto it = captures_.find(name);
        if (it == captures_.end()) return false;

        it->second->StopRec();
        return true;
    }

    float SoundMgr::getInputRmsLevel(std::string const& name) {
        auto it = captures_.find(name);
        if (it == captures_.end()) return 0.0f;
        return it->second->getRmsLevel();
    }

    float SoundMgr::getInputPeakLevel(std::string const& name) {
        auto it = captures_.find(name);
        if (it == captures_.end()) return 0.0f;
        return it->second->getPeakLevel();
    }

    size_t SoundMgr::getInputBufferSize(std::string const& name) {
        auto it = captures_.find(name);
        if (it == captures_.end()) return 0;
        return it->second->getBufferSize();
    }

    size_t SoundMgr::getInputRecBufferSize(std::string const& name) {
        auto it = captures_.find(name);
        if (it == captures_.end()) return 0;
        return it->second->getRecBufferSize();
    }

    bool SoundMgr::isInputDeviceValid(std::string const& name) const {
        auto it = captures_.find(name);
        if (it == captures_.end()) return false;
        return it->second->isValid();
    }


    // Gestión de dispositivos playbacks ----------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Popular vector con dispositivos disponibles
        std::vector<std::string> devlist;
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i) 
            devlist.push_back(pimpl_->pPlaybackDevInfos_[i].name);
        return devlist;
    }

    std::vector<std::string> SoundMgr::getManagedPlaybacks() const {
        if (!initialized_) return {};
        std::vector<std::string> pb;
        // Popular vector con dispositivos administrados
        for (auto& it : playbacks_)
            pb.push_back(it.first);
        return pb;
    }

    std::string SoundMgr::getDefaultPlaybackDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Buscar en vector de dispositivos
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i)
            if (pimpl_->pPlaybackDevInfos_[i].isDefault)
                return pimpl_->pPlaybackDevInfos_[i].name;

        /*else*/ return "";
    }

    void SoundMgr::listAvailablePlaybacks() const {
        for (std::string const& name : getAvailablePlaybacks())
            SYS_INFO("SoundMgr", "Playback: " + name);
    }

    bool SoundMgr::addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder) {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
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
        if (!selectedDeviceInfo) {
            SYS_WARN("SoundMgr","Failed to found device" + deviceName);
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
        playbacks_[deviceName] = std::move(apm);
        SYS_INFO("SoundMgr", "Playback loaded. ");

        return true;
    }

    bool SoundMgr::removePlaybackDevice(std::string const& name) {

        // comprobar si existe
        auto it = playbacks_.find(name);
        if (it == playbacks_.end()) {
            SYS_WARN("SoundMgr", "Selected playback device '" + name + "' not found.");
            return false;
        }
        
        // Detener las operaciones del módulo de reproducción
        it->second->stop();

        // Borrar el elemento del vector usando iterator
        playbacks_.erase(it);

        // Notificar y salir
        SYS_INFO("SoundMgr", "Deleted Playback.");
        return true;
    }


    // Ejecución y datos de playbacks -------------------------------------------------------

    bool SoundMgr::playbackTest() {

        if (!initialized_) {
            SYS_WARN("SoundMgr","Audio context not initialized.");
            return false;
        }

        // Actualizar dispositivos disponibles
        updateDevices();

        // Voy a probar tomando el dispositivo de audio predeterminado
        std::string defDevice = getDefaultPlaybackDevice();
        if (defDevice.empty()) return false;
        
        // Añade el nuevo dispositivo playback
        addPlaybackDevice(defDevice, "audioTest");

        // Confirma que se ha agregado bien
        auto it = playbacks_.find(defDevice);
        if (it == playbacks_.end()) 
            return false;

        // puntero al APM que acabamos de meter
        AudioPlaybackModule* ultimoAPM = it->second.get();
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
        removePlaybackDevice(defDevice);

        return true;
    }
    

#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    SoundMgr::SoundMgr()             {}
    SoundMgr::~SoundMgr()            {}
    bool SoundMgr::init()            { return false; }
    bool SoundMgr::stop()            { return false; }
    bool SoundMgr::updateDevices()   { return false; }

    // Capture Input ------------------------------------------------------------------------
    std::vector<std::string> SoundMgr::getAvailableInputs() const               { return {}; }
    std::string SoundMgr::getDefaultInputDevice() const                         { return {}; }
    bool        SoundMgr::addCaptureDevice(std::string const&, unsigned short)  { return false; }
    bool SoundMgr::removeInputDevice(unsigned short)                            { return false; }
    bool SoundMgr::startRec(unsigned short)                                     { return false; }
    bool SoundMgr::stopRec(unsigned short)                                      { return false; }
    float SoundMgr::getInputRmsLevel(unsigned short)                            { return 0.0f;  }

    size_t SoundMgr::getInputBufferSize(unsigned int)                     { return 0;     }    
    size_t SoundMgr::getInputRecBufferSize(unsigned int)                  { return 0;     }        
    bool   SoundMgr::isInputDeviceValid(unsigned short) const             { return false; }            

    // Playbacks ----------------------------------------------------------------------------
    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const    { return {}; }
    std::string SoundMgr::getDefaultPlaybackDevice() const              { return {}; }
    void        SoundMgr::listAvailablePlaybacks()                      { return;    }
    bool        SoundMgr::addPlaybackDevice(std::string const&, std::string const&) { return false; }
    bool        SoundMgr::removePlaybackDevice(unsigned short)          { return false; }
    bool        SoundMgr::playbackTest()                                { return false; }

#endif
