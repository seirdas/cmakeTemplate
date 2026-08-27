#include "sound/SoundMgr.hpp"
#include "sound/AudioCaptureModule.hpp"
#include "sound/AudioPlaybackModule.hpp"
#include "sound/PlayerAudio.hpp"
#include "sound/PlayerMorse.hpp"
#include "sound/PlayerTTS.hpp"
#include "sound/TTSCore.hpp"


// Macro de cmake al activar la librería
#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <memory>
    #include <chrono>
    #include <algorithm>
    #include "system/SystemMgr.hpp"
    #include "files/JsonMgr.hpp"            // Para conocer json
    #include "sound/ISoundObserver.hpp"     // Datos para mandar a patrón observador
    
    
    // Implementación de miembros de la clase de miniaudio (pimpl_)
    struct SoundMgr::Impl {
        ma_context      snd_context_;                       ///< Contexto (motor) de audio
        ma_device_info* pPlaybackDevInfos_  = nullptr;      ///< Información de dispositivos playback
        ma_uint32       PlaybackDevCount_   = 0;            ///< Número de dispositivos playback
        ma_device_info* pCaptureDevInfos_   = nullptr;      ///< Información de dispositivos de captura
        ma_uint32       captureDevCount_    = 0;            ///< Número de dispositivos de captura

        /**
         * @brief Constructor
         */
        inline Impl() {}
    };


    // General ------------------------------------------------------------------------------

    SoundMgr::SoundMgr() :
        pimpl_(std::make_unique<Impl>()),
        initialized_(false),
        running_(false),
        MAX_REINIT_ATTEMPTS(3),
        fallbackToDefault_(true),
        tts_(std::make_unique<TTSCore>()),
        enabledSmoothedValues_(true),
        attackCoeff_(0.5f),
        releaseCoeff_(0.1f)
    {

    }

    SoundMgr::~SoundMgr() {
        close();
    }


    // Inicialización -----------------------------------------------------------------------

    bool SoundMgr::init(void* config) {

        // No hacer nada si ya se ha iniciado
        if (initialized_) return true;
        SYS_INFO("SoundMgr", "Initializing sound context...");

        // Inicializar sistema de audio
        ma_result res = ma_context_init(NULL, 0, NULL, &pimpl_->snd_context_);
        initialized_ = (res == MA_SUCCESS) ? true : false;
        if (!initialized_) {
            SYS_ERROR("SoundMgr","Cannot initialize audio system (ma_context_init)");
            return false;
        }

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else
            SYS_WARN("SoundMgr","Cannot load config. Using default values");

        // Obtener dispositivos de captura/playback
        if (!updateDevices())
            SYS_WARN("SoundMgr","Failed to get playback devices");

        // Inicialización de TTSCore (en hilo para no bloquear)
        SYS_INFO("SoundMgr","Starting TTSCore async load...");
        initTTS_thread_ = std::thread([this, config]() {

                // Inyectar función de notify al TTSCore
                tts_->setCallback_onNotify([this](){
                    this->notify();
                });

                // Inicializar submódulo 
                if(!tts_->init(config))
                    SYS_WARN("TTSMgr","TTSCore FAIL");
                else SYS_INFO("TTSMgr","TTSCore OK");

                // Actualizar json con los datos de ttscore (si aplica)
                JsonMgr::instance().update();
            }
        );
    
        running_ = true;
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
        jsonMgr.get_or_set(cfg, "smoothedValues",       enabledSmoothedValues_);
        jsonMgr.get_or_set(cfg, "attack_coefficient",   attackCoeff_);
        attackCoeff_= std::clamp(attackCoeff_, minValue, maxValue);
        jsonMgr.get_or_set(cfg, "release_coefficient",  releaseCoeff_);
        releaseCoeff_= std::clamp(releaseCoeff_, minValue, maxValue);


        jsonMgr.get_or_set(cfg, "fallback_to_default",  fallbackToDefault_);


        /* CAPTURAS */
        // Recorrer elementos de capturas dentro del nodo json
        std::vector<json*> config_module_nodes = jsonMgr.getArrayElements(cfg, "Capture");
        for (json* node : config_module_nodes) {
            SYS_INFO("SoundMgr","Loading capture from config...");

            // Obtener el nombre para la inicialización
            std::string name = "";
            jsonMgr.get_or_set(node, "name", name);

            // Comprobar que el nombre no esté vacío
            if (name.empty()) {
                SYS_WARN("SoundMgr","Cannot initialize capture module: Name is empty");
                continue;
            }

            addCaptureDevice(node, name);
        }

        /* PLAYBACKS */
        // Recorrer elementos de playbacks dentro del nodo json (reutilizar json*)
        config_module_nodes = jsonMgr.getArrayElements(cfg, "Playbacks");
        for (json* node : config_module_nodes) {

            // Obtener el nombre para la inicialización
            std::string name = "";
            jsonMgr.get(node, "name", name);

            // Comprobar que el nombre no esté vacío
            if (name.empty()) {
                SYS_WARN("SoundMgr","Cannot initialize playback module: Name is empty");
                continue;
            }

            // Obtener el tipo de módulo (morse, tonos, ...)
            std::string type = "";
            jsonMgr.get_or_set(node, "type", type);

            // Transformar a minúsculas
            for (char &c : type)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); 

            // Agregar según el tipo de módulo
            if (type == "audio")
                addPlayerAudio(node);
            else if (type == "morse")
                addPlayerMorse(node);
            else if (type == "tts")
                addPlayerTTS(node);
            else 
                SYS_WARN("SoundMgr","config type error: only managed 'audio', 'morse' or 'tts' module types");
        }
    }

    bool SoundMgr::close() {
        // No hacer nada si ya se ha cerrado.
        if (!initialized_) return true;

        // Notifica el estado de cerrado (para threads, etc.)
        running_ = false;

        // Limpieza de módulos de captura
        if (!captures_.empty()) {
            SYS_INFO("SoundMgr", "Closing capture modules...");
            captures_.clear();
        }

        // Limpieza de módulos playback de audios
        if (!playersAudios_.empty()) {
            SYS_INFO("SoundMgr", "Closing Audio Player modules...");
            playersAudios_.clear();
        }
        
        // Limpieza de módulos playback de Morse
        if (!playersMorse_.empty()) {
            SYS_INFO("SoundMgr", "Closing Morse Player modules...");
            playersMorse_.clear();
        }
        
        // Limpieza de módulos playback de TTS
        if (!playersTTS_.empty()) {
            SYS_INFO("SoundMgr", "Closing TTS Player modules...");
            playersTTS_.clear();
        }

        // Limpieza del TTS
        if (tts_->isInitialized()) {
            SYS_INFO("AppController","Closing TTS manager...");
            tts_->close();
        }

        // Comprobación del hilo de inicialización del TTS
        if (initTTS_thread_.joinable()) {
            SYS_INFO("AppController","Closing TTS init thread...");
            initTTS_thread_.join();
        }

        // Desinicializar el contexto global
        SYS_INFO("SoundMgr", "Uninitializing sound context...");
        ma_context_uninit(&pimpl_->snd_context_);
        initialized_ = false;

        // Notificar y salir
        SYS_INFO("SoundMgr", "Sound system stopped successfully.");
        return true;
    }


    // Ejecución ----------------------------------------------------------------------------

    bool SoundMgr::updateDevices() {
        // Comprobar que el contexto está inicializado
        if (!initialized_) return false;

        // Obtener los dispositivos de reproducción y captura
        ma_result res = ma_context_get_devices(&pimpl_->snd_context_,
            &pimpl_->pPlaybackDevInfos_, &pimpl_->PlaybackDevCount_,
            &pimpl_->pCaptureDevInfos_, &pimpl_->captureDevCount_);

        if (res != MA_SUCCESS) {
            SYS_WARN("SoundMgr","Cannot retrieve data from audio devices: ma_context_get_devices error");
            return false;
        }

        // Si hay algun dispositivo con is_valid = false se reinicializa
        for (auto& [name, aim] : captures_) {
            if (aim->isValid()) continue;

            for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
                if (aim->getDeviceName() == pimpl_->pCaptureDevInfos_[i].name)
                    for (unsigned int tries = 0; tries < MAX_REINIT_ATTEMPTS; tries++)
                        if (aim->init(nullptr))
                            break;
        }

        // #TODO Hacer lo mismo para playbacks


        // Actualizar las listas de dispositivos disponibles/manejados
        update_available_inputs();
        update_available_playbacks();

        // Notificar a observadores
        notify();

        return true;
    }

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
        addPlayerAudio(nullptr, "playbackTest", defDevice);

        // Confirma que se ha agregado bien
        auto it = playersAudios_.find("playbackTest");
        if (it == playersAudios_.end()) 
            return false;

        // puntero al APM que acabamos de meter
        PlayerAudio* pm = getPlayerAudio("playbackTest");
        SYS_INFO("SoundMgr", "Testing module: '"
            + pm->getModuleName() + "' ("
            + pm->getDeviceName() + ")");

        /* reproducir */
        pm->playAudio("audio/ding.mp3", 100, true);
        pm->playAudio("audio/cat.mp3");

        SYS_INFO("SoundMgr", "Sleep for 500ms...");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // /* modificar mientras reproduce */
        pm->setVolume("audio/cat.mp3", 40);
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        pm->setVolume("click", 30);
        pm->setPitch("audio/cat.mp3", 1.7f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /* cortar música */
        pm->stop("audio/cat.mp3", true, 2000, 5000);  // (forzado)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        pm->stop("audio/ding.mp3");   // Esperar a que termine el wav (desactivar loop, forcestop = false)
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        // Opcional: limpiar el módulo (destruir sonidos)
        pm->close();

        // Remover el APM de la lista
        removePlayerAudio("playbackTest");

        return true;
    }


    // Módulos de Captura -------------------------------------------------------------------

    bool SoundMgr::addCaptureDevice(
        void*               config,
        std::string const&  moduleName, 
        std::string const&  deviceName) 
    {
        SYS_INFO("SoundMgr","Adding new Capture module...");
        bool res = add_module(config, moduleName, deviceName, captures_, true);

        // Comprobar que se ha añadido bien
        if (!res)
            return false;

        // Le pasa los parámetros globales del suavizado de valores
        AudioCaptureModule* acm = getCapture(moduleName);
        acm->enableSmoothedValues(enabledSmoothedValues_);
        acm->setSmoothAttackCoeff(attackCoeff_);
        acm->setSmoothReleaseCoeff(releaseCoeff_);

        return res; //<- true
    }

    bool SoundMgr::removeCaptureDevice(std::string const& moduleName) {
        return remove_module(moduleName, captures_);
    }

    AudioCaptureModule* SoundMgr::getCapture(std::string moduleName) const {
        auto it = captures_.find(moduleName);
        return (it != captures_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getCaptureModuleNames() const {
        std::vector<std::string> names;
        for (auto& it : captures_) 
            names.push_back(it.first);
        return names;
    }
    
    
    // Módulos PlayerAudio -------------------------------------------------------------

    bool SoundMgr::addPlayerAudio(
        void*               config,
        std::string const&  moduleName, 
        std::string const&  deviceName)
    {
        SYS_INFO("SoundMgr","Adding new PlayerAudio module...");
        return add_module(config, moduleName, deviceName, playersAudios_, false);
    }

    bool SoundMgr::removePlayerAudio(std::string const& moduleName) {
        return remove_module(moduleName, playersAudios_);
    }

    PlayerAudio* SoundMgr::getPlayerAudio(std::string moduleName) const {
        auto it = playersAudios_.find(moduleName);
        return (it != playersAudios_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getPlayerAudioNames() const {
        std::vector<std::string> names;
        for (auto& it : playersAudios_) 
            names.push_back(it.first);
        return names;
    }
    

    // Módulos PlayerMorse -------------------------------------------------------------

    bool SoundMgr::addPlayerMorse(
        void* config,
        std::string const& moduleName,
        std::string const& deviceName) 
    {
        SYS_INFO("SoundMgr","Adding new PlayerMorse module...");
        return add_module(config, moduleName, deviceName, playersMorse_, false);
    }

    bool SoundMgr::removePlayerMorse(std::string const& moduleName) {
       return remove_module(moduleName, playersMorse_);
    }

    PlayerMorse* SoundMgr::getPlayerMorse(std::string moduleName) const {
        auto it = playersMorse_.find(moduleName);
        return (it != playersMorse_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getPlayerMorseNames() const {
        std::vector<std::string> names;
        for (auto& it : playersMorse_) 
            names.push_back(it.first);
        return names;
    }
    

    // Módulos PlayerTTS -------------------------------------------------------------

    bool SoundMgr::addPlayerTTS(
        void* config,
        std::string const& moduleName,
        std::string const& deviceName) 
    {
        SYS_INFO("SoundMgr","Adding new PlayerTTS module...");
        bool res = add_module(config, moduleName, deviceName, playersTTS_, false);

        // Comprobar que se ha añadido bien
        if (!res)
            return false;

        // INYECCIÓN: Generar audio del texto usando TTSCore
        getPlayerTTS(moduleName)->setCallback_onTextToAudio([this](std::string const& modelName, std::string const& text) -> AudioData {
            return tts_->generate(modelName, text);
        });

        return res; //<- true
    }

    bool SoundMgr::removePlayerTTS(std::string const& moduleName) {
        return remove_module(moduleName, playersTTS_);
    }

    PlayerTTS* SoundMgr::getPlayerTTS(std::string moduleName) const {
        auto it = playersTTS_.find(moduleName);
        return (it != playersTTS_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getPlayerTTSNames() const {
        std::vector<std::string> names;
        for (auto& it : playersTTS_) 
            names.push_back(it.first);
        return names;
    }
    
    TTSCore* SoundMgr::getTTSCore() const {
        return tts_.get();
    }

    
    // Datos de dispositivos ----------------------------------------------------------------

    std::vector<std::string> SoundMgr::getAvailableCaptures() const {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        return available_captures_;
    }

    std::string SoundMgr::getDefaultCaptureDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Recorre todos los dispositivos de captura (para esto se tiene que usar la lista de infos)
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
        // Cuando encuentra el que tiene Default = true devuelve su nombre
            if (pimpl_->pCaptureDevInfos_[i].isDefault)
                return pimpl_->pCaptureDevInfos_[i].name;

        // Si no hay ninguno predeterminado, devuelve un ""
        /*else*/ return "";
    }

    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const {
        std::lock_guard<std::mutex> lock(available_playbacks_mtx_);
        return available_playbacks_;
    }

    std::string SoundMgr::getDefaultPlaybackDevice() const {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return {};
        }

        // Buscar en vector de dispositivos
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i)
            if (pimpl_->pPlaybackDevInfos_[i].isDefault)
                return pimpl_->pPlaybackDevInfos_[i].name;

        /*else*/ return "";
    }
    

    // Observadores -------------------------------------------------------------------------

    void SoundMgr::addObserver(ISoundObserver* obs) {
        observers_.push_back(obs);
    }


    // Notificar a observadores -------------------------------------------------------------

    void SoundMgr::notify() {
        // Estructura para mandar a través de la función
        OBS_SoundsData data = {};

        // Rellenar los datos de la estructura
        data.captures                   = getAvailableCaptures();
        data.players_audio              = getPlayerAudioNames();
        data.players_morse              = getPlayerMorseNames();
        data.players_tts                = getPlayerTTSNames();
        data.tts.available_models       = tts_->getAvailableModels();
        data.tts.loaded_models          = tts_->getLoadedModels();
        data.tts.num_available_models   = tts_->numAvailableModels();
        data.tts.num_loaded_models      = tts_->numLoadedModels();

        // Notificar a los observadores pasándoles la estructura
        for (auto* obs : observers_) {
            obs->onSoundsDataChanged(data);
        }
    }


    // Funciones internas auxiliares --------------------------------------------------------

    const void* SoundMgr::get_device_info(std::string& myDevName, bool isCapture) const {

        // Seleccionar la lista de dispositivos en función del parámetro de entrada (captures/playbacks)
        ma_uint32   devInfosSize = (isCapture) ? pimpl_->captureDevCount_   : pimpl_->PlaybackDevCount_;
        ma_device_info* devInfos = (isCapture) ? pimpl_->pCaptureDevInfos_  : pimpl_->pPlaybackDevInfos_;
        
        // Si encuentra un dispositivo con el nombre entero literal, no busca parecidos
        for (ma_uint32 i = 0; i < devInfosSize; ++i)
            if (myDevName == devInfos[i].name)  
                return &devInfos[i];

        // Si no ha encontrado nada literalmente igual y llega hasta aquí, busca parecidos (ignore case)
        ma_device_info* selectedDeviceInfo  = nullptr;  // Dispositivo a devolver
        std::size_t count    = 0;                       // Número de coincidencias
        std::string realDevName;                        // Nombre de dispositivo a sustituir si se encuentra
        std::string devNameLowercase = "";              // Nombre real del dispositivo en el bucle (minúsculas)
        std::string myDevNameLowercase = "";            // Nombre especificado por parámetro (minúsculas)

        // Convertir el nombre del dispositivo del parámetro a minúsculas
        myDevNameLowercase = myDevName;
        std::transform(myDevNameLowercase.begin(), myDevNameLowercase.end(), myDevNameLowercase.begin(),
                [](unsigned char c){ return std::tolower(c); });

        // Recorrer la lista de dispositivos (captures o playbacks, especificado al principio)
        for (ma_uint32 i = 0; i < devInfosSize; ++i) { 
            
            // Obtener el nombre real del dispositivo encontrado en esta iteracción en minúsculas
            devNameLowercase = devInfos[i].name;
            std::transform(devNameLowercase.begin(), devNameLowercase.end(), devNameLowercase.begin(),
                [](unsigned char c){ return std::tolower(c); });

            // Comprobar si el nombre del dispositivo coincide con el del parámetro (minúsculas)
            if (devNameLowercase.find(myDevNameLowercase) != std::string::npos) {
                
                // Guarda la info en el ma_device_info a devolver
                selectedDeviceInfo = &devInfos[i];

                // Guardar este nombre para sustituir el del parámetro después
                realDevName = devInfos[i].name; 

                // Suma el contador para localizar ambigüedades (si >1)
                count++;
            }
        }

        // Si hay varios que coinciden, es ambiguo: abortar
        if (count > 1) {
            SYS_WARN("SoundMgr","Cannot initialize audio input: Ambiguous name specified: '" + myDevName + "'");
            return nullptr;
        }

        // Si ha llegado hasta aquí, sustituir el nombre proporcionado por el real
        if (!realDevName.empty()) 
            myDevName = realDevName;

        // Devolver el dispositivo
        return selectedDeviceInfo;
    }

    const void* SoundMgr::get_playback_device_info(std::string& myDeviceName) const {
        return get_device_info(myDeviceName, false);
    }

    const void* SoundMgr::get_capture_device_info(std::string& myDeviceName) const {
        return get_device_info(myDeviceName, true);
    }

    template <typename MapT>
    bool SoundMgr::add_module(
        void*               config, 
        std::string const&  moduleName, 
        std::string const&  deviceName, 
        MapT&               map,
        bool                isCapture) 
    {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // Obtener la config y la instancia de jsonMgr (para config si aplica)
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Nombre a usar para el módulo
        std::string usedModuleName;
        {
            // Intentar obtener nombre del parametro (predominante)
            usedModuleName = moduleName;

            // Si se queda vacío, intentar obtener nombre de la config
            if (usedModuleName.empty() && config)
                jsonMgr.get(cfg, "name", usedModuleName);
            
            // Si se queda vacío, generar uno por defecto
            if (usedModuleName.empty())
                usedModuleName = "MODULE#" + std::to_string(rand());
        }
        
        // Nombre a usar para el dispositivo
        std::string usedDeviceName;
        {
            // Intentar obtener nombre del parametro (predominante)
            usedDeviceName = deviceName;

            // Si se queda vacío, intentar obtener nombre de la config
            if (usedDeviceName.empty() && config)
                jsonMgr.get(cfg, "device", usedDeviceName);
            
            // Si se queda vacío, fallback a default si habilitado
            if (usedDeviceName.empty() && fallbackToDefault_) {
                SYS_INFO("SoundMgr","Device name not specified: fallback to default");
                usedDeviceName = isCapture ? getDefaultCaptureDevice() : getDefaultPlaybackDevice();
            }
            
            // Si se queda vacío, salir
            if (usedDeviceName.empty()) {
                SYS_WARN("SoundMgr","Cannot obtain device name");
                return false;
            }
        }

        // Obtiene la información del dispositivo (ma_device_info)
        /* (Sobreescribe la variable realDeviceName por el nombre real) */
        std::string realDeviceName = usedDeviceName;
        const ma_device_info* selectedDeviceInfo = 
            static_cast<const ma_device_info*>(get_device_info(realDeviceName, isCapture));
        if (!selectedDeviceInfo) {
            SYS_WARN("SoundMgr", "Failed to find device: '" + usedDeviceName + "'");
            return false;
        }

        // Corrige la config con el nombre real completo del dispositivo encontrado
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Usar el tipo guardado en el std::unique_ptr del mapa
        using ModuleType = typename MapT::mapped_type::element_type;

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Creating new module: " + usedModuleName);
        std::unique_ptr<ModuleType> module = std::make_unique<ModuleType>(
            usedModuleName,
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing module...");
        if (!module->init(config, usedModuleName))
        {
            SYS_WARN("SoundMgr","Failed to initialize module");
            return false;
        }

        // Guardar en el mapa correspondiente del parámetro
        SYS_INFO("SoundMgr", "Adding module to map...");
        map[usedModuleName] = std::move(module);
        SYS_INFO("SoundMgr", "New module added: '" + usedModuleName + "' (" + realDeviceName + ")");

        // Notificar a observadores
        notify();

        return true;
    }

    template <typename MapT>
    bool SoundMgr::remove_module(std::string const& name, MapT& map) {

        // Comprobar si está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // Buscar el módulo en el mapa
        auto it = map.find(name);
        if (it == map.end()) {
            SYS_WARN("SoundMgr", "'" + name + "' not found.");
            return false;
        }

        // Cerrar y eliminar el módulo
        it->second->close();
        map.erase(it);
        SYS_INFO("SoundMgr", "Deleted '" + name + "'.");

        // Notificar a observadores
        notify();

        return true;
    }


    // Listas de dispositivos de audio ------------------------------------------------------

    void SoundMgr::update_available_inputs() {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        available_captures_.clear();
        for (unsigned int i = 0; i < pimpl_->captureDevCount_; ++i) 
            available_captures_.push_back(pimpl_->pCaptureDevInfos_[i].name);
    }

    void SoundMgr::update_available_playbacks() {
        std::lock_guard<std::mutex> lock(available_playbacks_mtx_);
        available_playbacks_.clear();
        for (unsigned int i = 0; i < pimpl_->PlaybackDevCount_; ++i) 
            available_playbacks_.push_back(pimpl_->pPlaybackDevInfos_[i].name);
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct SoundMgr::Impl {};

// General ------------------------------------------------------------------------------
SoundMgr::SoundMgr()                    { }
SoundMgr::~SoundMgr()                   { }
bool SoundMgr::init(void*)              { return false; }
bool SoundMgr::isInitialized() const    { return false; }
bool SoundMgr::close()                   { return false; }
bool SoundMgr::updateDevices()          { return false; }

// Dispositivos de Captura --------------------------------------------------------------
AudioCaptureModule* SoundMgr::getCapture(std::string captureName) const       { return nullptr;}
std::vector<std::string> SoundMgr::getAvailableCaptures() const             { return {}; }
std::vector<std::string> SoundMgr::getManagedCaptures() const               { return {}; }
bool        SoundMgr::isOnManagedCaptures(std::string const&) const         { return false; }
std::string SoundMgr::getDefaultCaptureDevice() const                       { return {}; }
bool SoundMgr::addCaptureDevice(void*,std::string const&,std::string const&){ return false; }
bool SoundMgr::removeCaptureDevice(std::string const&)                      { return false; }

// Dispositivos Playback ----------------------------------------------------------------
AudioPlaybackModule* SoundMgr::getPlayback(std::string captureName) const   { return nullptr; }
std::vector<std::string> SoundMgr::getAvailablePlaybacks() const { return {}; }
std::vector<std::string> SoundMgr::getManagedPlayerAudios() const   { return {}; }
bool SoundMgr::isOnManagedPlayerAudios(std::string const&) const    { return false; }
std::string SoundMgr::getDefaultPlaybackDevice() const           { return ""; }
void SoundMgr::listAvailablePlaybacks() const                    { return; }
bool SoundMgr::addPlayerAudio(void*,std::string const&,std::string const&, std::string const&) { return false; }
bool SoundMgr::removePlayerAudio(std::string const&)          { return false; }

// Ejecución y datos de playbacks -------------------------------------------------------
bool SoundMgr::playbackTest()                               { return false; }
bool SoundMgr::playMorse(std::string const&, std::string const&, std::string const&, unsigned short, bool) { return false;}
bool SoundMgr::stopTone(std::string const&) { return false;}

// Funciones internas auxiliares --------------------------------------------------------
const void* SoundMgr::get_device_info(std::string&, bool) const     { return nullptr; }
const void* SoundMgr::get_playback_device_info(std::string&) const  { return nullptr; }
const void* SoundMgr::get_capture_device_info(std::string&) const   { return nullptr; }

// Listas de dispositivos de audio ------------------------------------------------------
void SoundMgr::update_available_inputs()    { return; }
void SoundMgr::update_available_playbacks() { return; }

#endif
