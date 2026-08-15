#include "sound/SoundMgr.hpp"


// Macro de cmake al activar la librería
#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <memory>
    #include <chrono>
    #include <thread>
    #include <algorithm>
    #include "sound/AudioInputModule.hpp"
    #include "sound/AudioPlaybackModule.hpp"
    #include "system/SystemMgr.hpp"
    #include "files/JsonMgr.hpp"    // Para conocer json
    #include "datatypes/MorseDict.hpp"


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
        enabledSmoothedValues_(true),
        attackCoeff_(0.5),
        releaseCoeff_(0.1),
        morseUnitMs_(100),
        morseRayaMs_(300),
        morseEspacioEntreSimbolos_(100),
        morseEspacioEntreLetras_(300),
        morseSampleRate_(48000)
    {

    }

    SoundMgr::~SoundMgr() {
        close();
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
        jsonMgr.get_or_set(cfg, "smoothedValues",       enabledSmoothedValues_);
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
        std::vector<json*> playbackElements = jsonMgr.getArrayElements(cfg, "Playbacks");

        // usamos pb como puntero tipo json para recorrer todos los elementos de playback

        for (json* pb : playbackElements) {
            std::string name;
            jsonMgr.get(pb, "name", name);

            // if(name==CCAS)
            // TODO**

            if (name == "MORSE") {
                jsonMgr.get_or_set(pb, "unit_ms", morseUnitMs_);
                jsonMgr.get_or_set(pb, "sample_rate", morseSampleRate_);

                // Valores por defecto derivados de unit_ms (compatibilidad con configs antiguas)
                morseRayaMs_ = morseUnitMs_ * 3;
                jsonMgr.get_or_set(pb, "raya_ms", morseRayaMs_);

                morseEspacioEntreSimbolos_ = morseUnitMs_;
                jsonMgr.get_or_set(pb, "espacio_entre_simbolos_ms", morseEspacioEntreSimbolos_);

                morseEspacioEntreLetras_ = morseUnitMs_ * 3;
                jsonMgr.get_or_set(pb, "espacio_entre_letras_ms", morseEspacioEntreLetras_);

                initTonePools(pb);

                std::vector<json*> typeElements = jsonMgr.getArrayElements(pb, "Type");

                for (json* type : typeElements) {
                    std::string typeName;
                    jsonMgr.get(type, "name", typeName);


                    float        frequencyHz              = 1350.0f;
                    unsigned int espacioEntreMorse        = 7000;

                    jsonMgr.get_or_set(type, "frequency_hz",      frequencyHz);
                    jsonMgr.get_or_set(type, "espacioEntreMorse", espacioEntreMorse);

                    morseFrequencies_[typeName]  = frequencyHz;
                    espacioEntreMorse_[typeName] = espacioEntreMorse;

                }
            }
        }
    }

    bool SoundMgr::close() {
        // No hacer nada si ya se ha cerrado.
        if (!initialized_) return true;

        // Limpieza (destruir) los módulos creados
        SYS_INFO("SoundMgr", "Closing capture modules...");
        captures_.clear();
        SYS_INFO("SoundMgr", "Closing playback modules...");
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

        // Estos también, aunque aquí no son necesarios
        update_managed_inputs();
        update_managed_playbacks();

        return true;
    }


    // Dispositivos de Captura --------------------------------------------------------------

    AudioInputModule* SoundMgr::getCapture(std::string captureName) const {
        auto it = captures_.find(captureName);
        return (it != captures_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getAvailableCaptures() const {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        return available_inputs_;
    }

    std::vector<std::string> SoundMgr::getManagedCaptures() const {
        std::lock_guard<std::mutex> lock(managed_inputs_mtx_);
        return managed_inputs_;
    }

    bool SoundMgr::isOnManagedCaptures(std::string const& captureName) const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Recorre todos los dispositivos de captura
        for (std::string const& managedCaptureNames : managed_inputs_)
            if (managedCaptureNames == captureName)
                return true;

        /*else*/ return false;
    }

    std::string SoundMgr::getDefaultCaptureDevice() const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return {};

        // Recorre todos los dispositivos de captura (para esto se tiene que usar la lista de infos)
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i)
        // Cuando encuentra el que tiene Default = true devuelve su nombre
            if (pimpl_->pCaptureDevInfos_[i].isDefault)
                return pimpl_->pCaptureDevInfos_[i].name;

        // Si no hay nungún predeterminado, devuelve un ""
        /*else*/ return "";
    }

    void SoundMgr::listAvailableCaptures() const {
        for (std::string const& name : getAvailableCaptures())
            SYS_INFO("SoundMgr", "Capture: " + name);
    }
    
    bool SoundMgr::addCaptureDevice(
        void*               config,
        std::string const&  captureName, 
        std::string const&  deviceName) 
    {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // Intentar obtener nombre y dispositivo del param o de la config
        std::string usedDeviceName  = deviceName;   // Nombre completo del dispositivo
        std::string usedModuleName  = captureName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (usedDeviceName.empty() && usedModuleName.empty()) {
            if (!config) {
                SYS_WARN("SoundMgr","addCaptureDevice: Cannot retrieve capture device name.");
                return false;
            }
            // Intentar obtener del json el nombre del módulo y el nombre del dispositivo
            if (usedDeviceName.empty())
                jsonMgr.get(static_cast<json*>(config), "device", usedDeviceName);

            // Intentar obtener del json el nombre del dispositivo
            if (usedModuleName.empty())
                jsonMgr.get(static_cast<json*>(config), "name", usedModuleName);
        }

        // Comprobar si se ha obtenido bien el dispositivo
        if (usedDeviceName.empty()) {
            SYS_WARN("SoundMgr","addCaptureDevice: Cannot retrieve device name.");
            return false;
        }
        // Comprobación del nombre: Le ponemos un nombre random si no tiene
        if (usedModuleName.empty())
            usedModuleName = "CAPTURE#" + std::to_string(rand());
        
        // Obtiene la información del dispositivo (ma_device_info)
        /* (Sobreescribe la variable realDeviceName por el nombre real) */
        std::string realDeviceName = usedDeviceName;        // Nombre real del dispositivo
        const ma_device_info* selectedDeviceInfo 
            = reinterpret_cast<const ma_device_info*>(get_capture_device_info(realDeviceName)); // aquí sobreescribe
            
        // Si no encuentra ningún nombre sale con fallo
        if(!selectedDeviceInfo) {
            SYS_WARN("SoundMgr", "Failed to found device: '" + deviceName + "'"); 
            return false; 
        }

        // corrige la config con el nombre completo real para que quede guardado igual
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing capture module: " + realDeviceName);
        std::unique_ptr<AudioInputModule> aim = std::make_unique<AudioInputModule>(
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intenta inicializar
        SYS_INFO("SoundMgr", "Initializing capture module...");
        if(!aim->init(config, captureName))
        {
            SYS_WARN("SoundMgr","Failed to initialize capture module");
            return false;
        }

        // Le pasa los parámetros globales del suavizado de valores
        aim->enableSmoothedValues(enabledSmoothedValues_);
        aim->setSmoothAttackCoeff(attackCoeff_);
        aim->setSmoothReleaseCoeff(releaseCoeff_);

        // Insertar el nuevo aim inicializado en el vector de capturas
        captures_[usedModuleName] = std::move(aim);
        SYS_INFO("SoundMgr", "New input module added: '" + usedModuleName + "' (" + realDeviceName + ")");

        // Actualizar la lista de capturas gestionadas
        update_managed_inputs();

        return true;
    }

    bool SoundMgr::removeCaptureDevice(std::string const& captureName) {

        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // comprobar si existe
        auto it = captures_.find(captureName);
        if (it == captures_.end()) {
            SYS_WARN("SoundMgr", "Selected device '" + captureName + "' not found");
            return false;
        }

        // Detener las operaciones del módulo de reproducción
        it->second->close();

        // Notificar antes de borrar (para obtener el nombre)
        SYS_INFO("SoundMgr", "Deleting capture device: '" + it->second->getModuleName() + "'");

        // Borrar el elemento del vector usando iterator
        captures_.erase(it);

        // Actualizar la lista de capturas gestionadas
        SYS_INFO("SoundMgr", "Capture device deleted");
        update_managed_inputs();
        return true;
    }
    

    // Dispositivos Playback ----------------------------------------------------------------

    AudioPlaybackModule* SoundMgr::getPlayback(std::string captureName) const {
        auto it = playbacks_.find(captureName);
        return (it != playbacks_.end() && it->second) ? it->second.get() : nullptr;
    }

    std::vector<std::string> SoundMgr::getAvailablePlaybacks() const {
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        return available_playbacks_;
    }

    std::vector<std::string> SoundMgr::getManagedPlaybacks() const {
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        return managed_playbacks_;
    }

    bool SoundMgr::isOnManagedPlaybacks(std::string const& playbackName) const {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // Recorre todos los dispositivos de playback
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i)
            if (pimpl_->pPlaybackDevInfos_[i].name == playbackName)
                return true;

        /*else*/ return false;
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

    void SoundMgr::listAvailablePlaybacks() const {
        for (std::string const& name : getAvailablePlaybacks())
            SYS_INFO("SoundMgr", "Playback: " + name);
    }

    bool SoundMgr::addPlaybackDevice(
        void*               config,
        std::string const&  playbackName, 
        std::string const&  deviceName,
        std::string const&  AudioFilesFolder)
    {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr","Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // Intentar obtener nombre y dispositivo del param o de la config
        std::string usedDeviceName  = deviceName;    // Nombre completo del dispositivo
        std::string usedModuleName  = playbackName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (usedDeviceName.empty() && usedModuleName.empty()) {
            if (!config) {
                SYS_WARN("SoundMgr","addCaptureDevice: Cannot retrieve capture device name.");
                return false;
            }
            // Intentar obtener del json el nombre del módulo y el nombre del dispositivo
            if (usedDeviceName.empty())
                jsonMgr.get(static_cast<json*>(config), "device", usedDeviceName);

            // Intentar obtener del json el nombre del dispositivo
            if (usedModuleName.empty())
                jsonMgr.get(static_cast<json*>(config), "name", usedModuleName);
        }

        // Comprobar si se ha obtenido bien el dispositivo
        if (usedDeviceName.empty()) {
            SYS_WARN("SoundMgr","addPlaybackDevice: Cannot retrieve device name.");
            return false;
        }
        // Comprobación del nombre: Le ponemos un nombre random si no tiene
        if (usedModuleName.empty())
            usedModuleName = "PLAYBACK#" + std::to_string(rand());
        
        // Obtiene la información del dispositivo (ma_device_info)
        /* (Sobreescribe la variable realDeviceName por el nombre real) */
        std::string realDeviceName = usedDeviceName;        // Nombre real del dispositivo
        const ma_device_info* selectedDeviceInfo 
            = static_cast<const ma_device_info*>(get_playback_device_info(realDeviceName)); // aquí sobreescribe
            
        // Si no encuentra ningún nombre sale con fallo
        if(!selectedDeviceInfo) {
            SYS_WARN("SoundMgr", "Failed to found device: '" + deviceName + "'"); 
            return false; 
        }

        // Corrige la config con el nombre real completo del dispositivo encontrado
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing playback module: " + usedModuleName);
        std::unique_ptr<AudioPlaybackModule> apm = std::make_unique<AudioPlaybackModule>(
            &pimpl_->snd_context_,
            selectedDeviceInfo,
            AudioFilesFolder
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback module...");
        if (!apm->init(config, playbackName))
        {
            SYS_WARN("SoundMgr","Failed to initialize playback module");
            return false;
        }

        // Insertar el nuevo apm inicializado en el vector de playback
        playbacks_[usedModuleName] = std::move(apm);
        SYS_INFO("SoundMgr", "New playback module added: '" + usedModuleName + "' (" + realDeviceName + ")");

        // Actualizar la lista de playbacks gestionados
        update_managed_playbacks();

        return true;
    }

    bool SoundMgr::removePlaybackDevice(std::string const& playbackName) {
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // comprobar si existe
        auto it = playbacks_.find(playbackName);
        if (it == playbacks_.end()) {
            SYS_WARN("SoundMgr", "Selected playback device '" + playbackName + "' not found.");
            return false;
        }

        // Detener las operaciones del módulo de reproducción
        it->second->close();

        // Notificar antes de borrar (para obtener el nombre)
        SYS_INFO("SoundMgr", "Deleting playback device: '" + it->second->getModuleName() + "'");

        // Borrar el elemento del vector usando iterator
        playbacks_.erase(it);

        // Notificar y salir
        SYS_INFO("SoundMgr", "Deleted Playback.");
        update_managed_playbacks();
        return true;
    }

    int SoundMgr::initTonePools(void* playbackConfig) {
        if(!playbackConfig)
        return 0;

        json* pb = static_cast<json*>(playbackConfig);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Nombre del grupo
        std::string groupName; 
        jsonMgr.get(pb, "name", groupName); 

        // Recorrer los dispositivos definidos en "device" para este grupo
        std::vector<json*> deviceElements = jsonMgr.getArrayElements(pb, "device");

        int registered = 0;
        for (json* dev : deviceElements) {
            std::string deviceName;
            jsonMgr.get(dev, "name", deviceName);

            if (deviceName.empty())
                continue;

            // #TODO Revisar. addPlaybackDevice no devuelve nombre

            // // Registrarlo como playback real
            // std::string registeredName = addPlaybackDevice(deviceName, "");
            // if (!registeredName.empty()) {
            //     tonePools_[groupName].push_back(registeredName);
            //     registered++;
            // }
        }

        return registered;
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
        addPlaybackDevice(nullptr, "playbackTest", defDevice);

        // Confirma que se ha agregado bien
        auto it = playbacks_.find("playbackTest");
        if (it == playbacks_.end()) 
            return false;

        // puntero al APM que acabamos de meter
        AudioPlaybackModule* ultimoAPM = it->second.get();
        SYS_INFO("SoundMgr", "Testing module: '"
            + ultimoAPM->getModuleName() + "' ("
            + ultimoAPM->getDeviceName() + ")");

        /* reproducir */
        ultimoAPM->playAudio("audio/ding.mp3", 100, true);
        ultimoAPM->playAudio("audio/cat.mp3");

        SYS_INFO("SoundMgr", "Sleep for 500ms...");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // /* modificar mientras reproduce */
        ultimoAPM->setVolume("cat", 40);
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        ultimoAPM->setVolume("click", 0.3f);
        ultimoAPM->setPitch("cat", 1.7f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /* cortar música */
        ultimoAPM->stopAudio("cat", true);  // (forzado)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        ultimoAPM->stopAudio("ding");   // Esperar a que termine el wav (desactivar loop, forcestop = false)
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        // Opcional: limpiar el módulo (destruir sonidos)
        ultimoAPM->close();

        // Remover el APM de la lista
        removePlaybackDevice(defDevice);

        return true;
    }

    bool SoundMgr::playMorse(std::string const& toneLabel, std::string const& tipo, std::string const& texto, unsigned short volume, bool loop){

        // 1. Buscar los parámetros del tipo (frecuencia, espacio)
        if (morseFrequencies_.find(tipo) == morseFrequencies_.end() ||
            espacioEntreMorse_.find(tipo) == espacioEntreMorse_.end()){
            SYS_WARN("SoundMgr", "playMorse: tipo desconocido '" + tipo + "'");
            return false; 
        }
        float           frequencyHz       = morseFrequencies_.at(tipo);
        unsigned int    espacioEntreMorse = espacioEntreMorse_.at(tipo); 

        // 2. Buscar el pool de playbacks del grupo MORSE
        TonePoolsList::iterator poolIt = tonePools_.find("MORSE"); 
        if (poolIt == tonePools_.end() || poolIt->second.empty()){
            SYS_WARN("SoundMgr", "playMorse: no hay playbacks registrados para MORSE");
            return false; 
        }

        // 3. Buscar el primer playback del pool que esté libre
        for (std::string const& playbackName : poolIt->second){
            PlaybacksList::iterator it = playbacks_.find(playbackName);
            if (it == playbacks_.end())
            continue; 

            if(!it->second->isBusy()) {
                return it->second->playMorse(texto, toneLabel, frequencyHz, morseUnitMs_, morseRayaMs_, morseEspacioEntreSimbolos_, morseEspacioEntreLetras_, morseSampleRate_, espacioEntreMorse, volume, loop);
            }
        }

        // 4. Si no hay ninguno libre, descartar y avisar
        SYS_WARN("SoundMgr", "playMorse: no hay ningun playback libre para reproducir '" + texto + "'");
        return false; 
    }

    bool SoundMgr::stopTone(std::string const& toneLabel) {

       TonePoolsList::iterator poolIt = tonePools_.find("MORSE");
       if (poolIt == tonePools_.end())
           return false;

       for (std::string const& playbackName : poolIt->second) {
           PlaybacksList::iterator it = playbacks_.find(playbackName);
           if (it == playbacks_.end())
               continue;

           if (it->second->isPlaying(toneLabel)) {
               it->second->stopAudio(toneLabel, true);
               return true;
           }
       }

       return false; 
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


    // Listas de dispositivos de audio ------------------------------------------------------

    void SoundMgr::update_available_inputs() {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        available_inputs_.clear();
        for (unsigned int i = 0; i < pimpl_->captureDevCount_; ++i) 
            available_inputs_.push_back(pimpl_->pCaptureDevInfos_[i].name);
    }

    void SoundMgr::update_managed_inputs() {
        std::lock_guard<std::mutex> lock(managed_inputs_mtx_);
        managed_inputs_.clear();
        for (auto& [name, aim] : captures_)
            managed_inputs_.push_back(name);
    }

    void SoundMgr::update_available_playbacks() {
        std::lock_guard<std::mutex> lock(available_playbacks_mtx_);
        available_playbacks_.clear();
        for (unsigned int i = 0; i < pimpl_->PlaybackDevCount_; ++i) 
            available_playbacks_.push_back(pimpl_->pPlaybackDevInfos_[i].name);
    }

    void SoundMgr::update_managed_playbacks() {
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        managed_playbacks_.clear();
        for (auto& [name, apm] : playbacks_)
            managed_playbacks_.push_back(name);
    }


    // Morse ---------------------------------------------------------------------------------

    std::vector<float> SoundMgr::generate_morse(std::string const& tipo, std::string const& texto) const {

        std::vector<float> audio;

        // Comprobar el tipo y sus valores de espacio y frecuencia
        if (morseFrequencies_.find(tipo) == morseFrequencies_.end() ||
            espacioEntreMorse_.find(tipo) == espacioEntreMorse_.end()) {
            SYS_WARN("SoundMgr", "generateMorse: tipo desconocido '" + tipo + "'");
            return audio;
        }

        // Guardar esos valores (y la unidad, compartida por todos los tipos)
        float        frequencyHz       = morseFrequencies_.at(tipo);
        unsigned int espacioEntreMorse = espacioEntreMorse_.at(tipo);
        unsigned int unitMs            = morseUnitMs_;

        // Recorrer el texto que nos dan, letra a letra
        for (size_t c = 0; c < texto.size(); ++c) {

            int letra = std::toupper(static_cast<unsigned char>(texto[c]));

            // Espacio: separación entre palabras
            if (letra == ' ') {
                size_t silentSamples = morseSampleRate_ * espacioEntreMorse / 1000;
                for (size_t i = 0; i < silentSamples; ++i)
                    audio.push_back(0.0f);
                continue;
            }

            // Letra no soportada por el diccionario: se ignora
            if (MORSE_DICT.find(letra) == MORSE_DICT.end())
                continue;

            std::string code = MORSE_DICT.at(letra);

            // 4. Generar el pitido de cada letra: puntos, rayas y espacio entre símbolos
            for (size_t s = 0; s < code.size(); ++s) {

                // Punto = 1 unidad, raya = 3 unidades
                unsigned int toneMs = (code[s] == '-') ? unitMs * 3 : unitMs;
                size_t toneSamples = morseSampleRate_ * toneMs / 1000;

                // 5. Generar el tono (onda senoidal) y guardarlo en audio
                for (size_t i = 0; i < toneSamples; ++i) {
                    float t = static_cast<float>(i) / morseSampleRate_;
                    audio.push_back(sin(2.0f * 3.14159265f * frequencyHz * t));
                }

                // Silencio entre símbolos de la misma letra (1 unidad)
                if (s + 1 < code.size()) {
                    size_t gapSamples = morseSampleRate_ * unitMs / 1000;
                    for (size_t i = 0; i < gapSamples; ++i)
                        audio.push_back(0.0f);
                }
            }

            // Espacio entre letras de la misma palabra (3 unidades)
            if (c + 1 < texto.size() && texto[c + 1] != ' ') {
                size_t gapSamples = morseSampleRate_ * unitMs * 3 / 1000;
                for (size_t i = 0; i < gapSamples; ++i)
                    audio.push_back(0.0f);
            }
        }

        return audio;
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

// Capture Input ------------------------------------------------------------------------
std::vector<std::string> SoundMgr::getAvailableCaptures() const             { return {}; }
std::vector<std::string> SoundMgr::getManagedCaptures() const               { return {}; }
bool        SoundMgr::isOnManagedCaptures(std::string const&) const         { return false; }
std::string SoundMgr::getDefaultCaptureDevice() const                       { return {}; }
void SoundMgr::listAvailableCaptures() const                                { return; }
bool SoundMgr::addCaptureDevice(void*,std::string const&,std::string const&){ return false; }
bool SoundMgr::removeCaptureDevice(std::string const&)                      { return false; }

// Ejecución y datos en dispositivos de captura -----------------------------------------
bool   SoundMgr::startRec(std::string const&) const              { return false; }
bool   SoundMgr::stopRec(std::string const&) const               { return false; }
float  SoundMgr::getInputRmsLevel(std::string const&) const      { return 0; }
float  SoundMgr::getInputPeakLevel(std::string const&) const     { return 0; }
bool   SoundMgr::isInputDeviceValid(std::string const&) const    { return false; }
size_t SoundMgr::getInputRecBufferSize(std::string const&) const { return 0; }
size_t SoundMgr::getInputBufferSize(std::string const&) const    { return 0; }

// Gestión de dispositivos playbacks ----------------------------------------------------
std::vector<std::string> SoundMgr::getAvailablePlaybacks() const { return {}; }
std::vector<std::string> SoundMgr::getManagedPlaybacks() const   { return {}; }
bool SoundMgr::isOnManagedPlaybacks(std::string const&) const    { return false; }
std::string SoundMgr::getDefaultPlaybackDevice() const           { return ""; }
void SoundMgr::listAvailablePlaybacks() const                    { return; }
bool SoundMgr::addPlaybackDevice(void*,std::string const&,std::string const&) { return false; }
bool SoundMgr::removePlaybackDevice(std::string const&)          { return false; }

// Ejecución y datos de playbacks -------------------------------------------------------
bool SoundMgr::playbackTest()                               { return false; }

// Funciones internas auxiliares --------------------------------------------------------
void* SoundMgr::get_device_info(std::string*, bool) const    { return nullptr; }
void* SoundMgr::get_playback_device_info(std::string*) const  { return nullptr; }
void* SoundMgr::get_capture_device_info(std::string*) const   { return nullptr; }

// Listas de dispositivos de audio ------------------------------------------------------
void SoundMgr::update_available_inputs()    { return; }
void SoundMgr::update_managed_inputs()      { return; }
void SoundMgr::update_available_playbacks() { return; }
void SoundMgr::update_managed_playbacks()   { return; }

// Morse --------------------------------------------------------------------------------
std::vector<float> SoundMgr::generate_morse(std::string const&, std::string const&) const  { return {}; }

#endif

