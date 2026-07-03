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
    #include "files/JsonMgr.hpp"    // Para conocer json


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

    bool SoundMgr::init(void* config) {

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

        //carga la configuración si existe, despues de iniciar el context
        if(config) 
            loadConfig(config);

        // Obtener dispositivos de captura/playback
        if (!updateDevices())
            SYS_WARN("SoundMgr","Failed to get playback devices.");

        return ctx_initialized_;
    }

    void SoundMgr::loadConfig(void* config) {
        if (!config) 
            return;
            
        // Se considera que la configuración se pasa como json    
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // obtener un array de punteros json
        std::vector<json*> config_elements = jsonMgr.getArrayElements(cfg, "Capture");

        // bucle que recorre los elementos de dentro del nodo
        for (short i=0; i < config_elements.size(); i++) 
            addCaptureDevice(config_elements[i]);
        
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

    if (res != MA_SUCCESS) return false;

    std::string name; 
    // Si hay algun dispositivo con is_valid = false se reinicializa
    for (auto& aim : inputs_) {
        name = aim->deviceName(); 
        if (aim->isValid()) continue;

        
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
            if (aim->deviceName() == pimpl_->pCaptureDevInfos_[i].name)
                for (unsigned int tries = 0; tries < MAX_REINIT_ATTEMPTS; tries++) // contador
                   if (aim->init(nullptr))
                        break;
                        
    }

    return true;
}


    // Capture Input --------------------------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailableInputs() const {

        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        // Crea un vector de strings devlist
        std::vector<std::string> devlist;
        // Recorre los dispositivos de captura que miniaudio encuentra
        // Se guardan en captureDevCount
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) 
            devlist.push_back(pimpl_->pCaptureDevInfos_[i].name);

        // Devolver la lista de dispositivos
        return devlist;
    }
 
    std::string SoundMgr::getDefaultInputDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!ctx_initialized_) return {};

        // Recorre todos los dispositivos de captura 
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
        // Cuando encuentra el que tiene Default = true devuelve su nombre
            if (pimpl_->pCaptureDevInfos_[i].isDefault)
                return pimpl_->pCaptureDevInfos_[i].name;

                // Si no hay nungún predeterminado, devuelve un ""
        /*else*/ return "";
    }
    
    bool SoundMgr::addCaptureDevice(void* config) {
     
        // Comprobar que el contexto está inicializado
        if (!ctx_initialized_){
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false; 
        }

        // Comprobar que existe la config
        if (!config) {
            SYS_WARN("SoundMgr","Cannot initialize audio input: empty config.");
            return false;
        }

        // Obtener el nombre de la config
        std::string device = "";
        JsonMgr& jsonMgr = JsonMgr::instance();
        jsonMgr.get_or_set(static_cast<json*>(config), "device", device);

        // Comprobación del nombre
        if (device.empty()) {
            SYS_WARN("SoundMgr","addCaptureDevice: Device name is empty.");
            return false;
        }

        // Refrescar lista de dispositivos disponibles
        updateDevices();

        // bucle para encontrar el dispositivo cuyo nombre real contenga lo escrito en la config
        short count = 0;
        ma_device_info* selectedDeviceInfo = nullptr;                      // puntero al dispositivo encontrado, empieza en null (ninguno encontrado aún)
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) {     // recorre todos los dispositivos de captura reales, uno a uno
            std::string realName = pimpl_->pCaptureDevInfos_[i].name;  // coge el nombre real del dispositivo 
            if (realName.find(device) != std::string::npos) {         // comprueba si "device" aparece dentro de "realName" (npos = "no encontrado")
                selectedDeviceInfo = &pimpl_->pCaptureDevInfos_[i];    // si coincide, guarda un puntero a ese dispositivo
                count++;                                                 // y sale del bucle, ya no hace falta seguir buscando
            }
        }

        // Se han encontrado varios con el mismo nombre
        if (count > 1) {
            SYS_WARN("SoundMgr","Cannot initialize audio input: Ambiguous name specified.");
            return false;
        }

        
        // Si no encuentra ningún nombre salta fallo
        if(!selectedDeviceInfo){
            SYS_WARN("SoundMgr", "Failed to found device with name"); 
            return false; 
        }

        //corrige la config con el nombre completo real para que quede guardado igual
        device = selectedDeviceInfo->name; 
        jsonMgr.set(static_cast<json*>(config), "device", device);
            
        SYS_INFO("SoundMgr", "Initializing capture device:" + device); 

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

        // El micrófono que acabas de crear (aim) lo metes en la lista de micrófonos (inputs_).
        inputs_.push_back(std::move(aim));
        SYS_INFO("SoundMgr", "New input device loaded.");

        return true;
    }
    
    bool SoundMgr::addCaptureDevice(std::string const& name) {
        
        // Comprobar que el contexto está inicializado
        if (!ctx_initialized_){
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false; 
        }

        // Refrescar lista de dispositivos disponibles
        updateDevices(); 

        // bucle para encontrar el nombre que hemos seleccionado
        ma_device_info* selectedDeviceInfo = nullptr;
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) {
            if (name == pimpl_->pCaptureDevInfos_[i].name) {
                selectedDeviceInfo = &pimpl_->pCaptureDevInfos_[i];
                break;
            }
        }
        
        // Si no encuentra ningún nombre salta fallo
        if(!selectedDeviceInfo){
            SYS_WARN("SoundMgr", "Failed to found device with name"); 
            return false; 
        }
    
        SYS_INFO("SoundMgr", "Initializing capture device:" + name); 

        // Crear AudioInputModule
        std::unique_ptr<AudioInputModule> aim = std::make_unique<AudioInputModule>(
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intenta inicializar el AudioInputModule
        if(!aim->init(nullptr))
        {
            SYS_WARN("SoundMgr","Failed to initialize capture.");
            return false;
        }

        // El micrófono que acabas de crear (aim) lo metes en la lista de micrófonos (inputs_).
        inputs_.push_back(std::move(aim));
        SYS_INFO("SoundMgr", "New input device loaded.");

        return true;
    }

    bool SoundMgr::removeInputDevice(unsigned short index) {

        // comprobar si existe
        if (index >= inputs_.size()) {
            SYS_WARN("SoundMgr","Selected index " + std::to_string(index) +
                " out of bounds (" + std::to_string(inputs_.size()) + ")");;
            return false;
        }
        
        // Obtener el módulo de reproducción
        AudioInputModule* aim = inputs_[index].get();

        // Detener las operaciones del módulo de reproducción
        aim->stop();

        // Borrar el elemento del vector usando iterator
        inputs_.erase(inputs_.begin() + index);

        // Notificar y salir
        SYS_INFO("SoundMgr", "Deleted Capture Device.");
        return true;
    }

    bool SoundMgr::startRec_snd(unsigned short index){
        if (index >= inputs_.size()) return false;

        std::string filename = "grabacion_" + std::to_string(index);
        inputs_[index]->StartRec(filename);

        return true;
    }

    bool SoundMgr::stopRec_snd(unsigned short index){
        if (index >= inputs_.size()) return false;

        AudioInputModule* aim = inputs_[index].get();
        aim->StopRec();

        return true;
    }

    float SoundMgr::getInputRmsLevel(unsigned short index) {
    if (index >= inputs_.size()) return 0.0f;
    return inputs_[index]->getRmsLevel();
    }

    float SoundMgr::getInputPeakLevel(unsigned short index) {
    if (index >= inputs_.size()) return 0.0f;
    return inputs_[index]->getPeakLevel();
    }

    size_t SoundMgr::getInputBufferSize(unsigned int index) {
        if (index >= inputs_.size()) return 0;
        return inputs_[index]->getBufferSize();
    }

    size_t SoundMgr::getInputRecBufferSize(unsigned int index)
    {
        if (index >= inputs_.size()) return 0;
        return inputs_[index]->getRecBufferSize();
    }

    bool SoundMgr::isInputDeviceValid(unsigned short index) const 
    {
        // Comprueba que el índice que pides existe.
        if (index >= inputs_.size()) return false;
        // comprueba si es valido
        return inputs_[index]->isValid();
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
        for (std::string const& name : getAvailablePlaybacks())
            SYS_INFO("SoundMgr", "Playback: " + name);
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
    bool SoundMgr::startRec_snd(unsigned short)                                 { return false; }
    bool SoundMgr::stopRec_snd(unsigned short)                                  { return false; }
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
