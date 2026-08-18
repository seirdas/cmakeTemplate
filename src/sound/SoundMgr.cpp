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
    #include "files/JsonMgr.hpp"    // Para conocer json

    
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
        tts_(std::make_unique<TTSCore>()),
        enabledSmoothedValues_(true),
        attackCoeff_(0.5),
        releaseCoeff_(0.1)
    {

    }

    SoundMgr::~SoundMgr() {
        close();
    }


    // Inicialización -----------------------------------------------------------------------

    bool SoundMgr::init(void* config) {

        // No hacer nada si ya se ha iniciado
        if (initialized_) return true;
        SYS_INFO("SoundMgr", "Initializating sound context...");

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

                // Inyectar función de notify al TTSCore ( #TODO )
                // tts_->setCallback_onNotify([this](){
                //     this->notify();
                // });

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


        /* CAPTURAS */
        // Recorrer elementos de capturas dentro del nodo json
        std::vector<json*> config_module_nodes = jsonMgr.getArrayElements(cfg, "Capture");
        for (json* node : config_module_nodes) {
            SYS_INFO("SoundMgr","Loading capture from config...");

            // Obtener el nombre para la inicialización
            std::string name = "";
            jsonMgr.get(node, "name", name);

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

        // Identificar el tipo de módulo
        if (name == "MORSE") {
            addPlayerMorse(nullptr, name, "");   // el fallback interno elige el primer dispositivo libre

            PlayerMorse* morse = getPlayerMorse(name);
            if (morse) {
                float frequencyHz = 1350.0f;
                jsonMgr.get_or_set(node, "frequency_hz", frequencyHz);
                morse->setToneFrequency(frequencyHz);

                unsigned int puntoMs = 100;
                jsonMgr.get_or_set(node, "punto_ms", puntoMs);
                morse->setPuntoMs(puntoMs);

                unsigned int rayaMs = 300;
                jsonMgr.get_or_set(node, "raya_ms", rayaMs);
                morse->setRayaMs(rayaMs);

                unsigned int espacioSimbolos = 100;
                jsonMgr.get_or_set(node, "espacio_entre_simbolos_ms", espacioSimbolos);
                morse->setEspacioEntreSimbolos(espacioSimbolos);

                unsigned int espacioLetras = 300;
                jsonMgr.get_or_set(node, "espacio_entre_letras_ms", espacioLetras);
                morse->setEspacioEntreLetras(espacioLetras);

                unsigned int sampleRate = 48000;
                jsonMgr.get_or_set(node, "sample_rate", sampleRate);
                morse->setSampleRate(sampleRate);
            }
        }
    // #TODO: else if (name == "CCAS" / "TCAS" / "TONES") -> addPlayerAudio(...)
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
        pm->setVolume("cat", 40);
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        pm->setVolume("click", 30);
        pm->setPitch("cat", 1.7f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /* cortar música */
        pm->stopAudio("cat", true, 2000, 5000);  // (forzado)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        pm->stopAudio("ding");   // Esperar a que termine el wav (desactivar loop, forcestop = false)
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
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr", "Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // Intentar obtener nombre y dispositivo del param o de la config
        std::string usedDeviceName  = deviceName;   // Nombre completo del dispositivo
        std::string usedModuleName  = moduleName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (usedDeviceName.empty() || usedModuleName.empty()) {
            if (!config) {
                SYS_WARN("SoundMgr","Cannot retrieve module name");
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
            = static_cast<const ma_device_info*>(get_capture_device_info(realDeviceName)); // aquí sobreescribe
            
        // Si no encuentra ningún nombre sale con fallo
        if(!selectedDeviceInfo) {
            SYS_WARN("SoundMgr", "Failed to found device: '" + usedDeviceName + "'"); 
            return false; 
        }

        // corrige la config con el nombre completo real para que quede guardado igual
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing capture module: " + realDeviceName);
        std::unique_ptr<AudioCaptureModule> aim = std::make_unique<AudioCaptureModule>(
            usedModuleName,
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intenta inicializar
        SYS_INFO("SoundMgr", "Initializing capture module...");
        if(!aim->init(config, usedModuleName))
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

        return true;
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
        // Comprobar que el contexto está inicializado
        if (!initialized_) {
            SYS_WARN("SoundMgr","Audio context not initialized.");
            return false;
        }

        // Refrescar la lista de dispositivos disponibles
        updateDevices();

        // Intentar obtener nombre y dispositivo del param o de la config
        std::string usedDeviceName  = deviceName;    // Nombre completo del dispositivo
        std::string usedModuleName  = moduleName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (usedDeviceName.empty() || usedModuleName.empty()) {
            if (!config) {
                SYS_WARN("SoundMgr","Cannot retrieve module name");
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
            SYS_WARN("SoundMgr","Cannot retrieve device name");
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
            SYS_WARN("SoundMgr", "Failed to found device: '" + usedDeviceName + "'"); 
            return false; 
        }

        // Corrige la config con el nombre real completo del dispositivo encontrado
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing playback module: " + usedModuleName);
        std::unique_ptr<PlayerAudio> apm = std::make_unique<PlayerAudio>(
            usedModuleName,
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback module...");
        if (!apm->init(config, usedModuleName))
        {
            SYS_WARN("SoundMgr","Failed to initialize playback module");
            return false;
        }

        // Guardar en PlayerTTS en la lista de Players
        playersAudios_[usedModuleName] = std::move(apm);
        SYS_INFO("SoundMgr", "New playback module added: '" + usedModuleName + "' (" + realDeviceName + ")");

        return true;
    }

    bool SoundMgr::removePlayerAudio(std::string const& moduleName) {
        return remove_module(moduleName, playersAudios_);
    }

    PlayerAudio* SoundMgr::getPlayerAudio(std::string moduleName) const {
        auto it = playersAudios_.find(moduleName);
        return (it != playersAudios_.end() && it->second) ? it->second.get() : nullptr;
    }


    // Módulos PlayerMorse -------------------------------------------------------------

    bool SoundMgr::addPlayerMorse(
        void* config,
        std::string const& moduleName,
        std::string const& deviceName) 
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
        std::string usedModuleName  = moduleName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Intentar obtener del json el nombre del módulo y el nombre del dispositivo
        if (config) {
            if (usedDeviceName.empty())
                jsonMgr.get(static_cast<json*>(config), "device", usedDeviceName);
            if (usedModuleName.empty())
                jsonMgr.get(static_cast<json*>(config), "name", usedModuleName);
        }

        // Si sigue vacío, coger el primer dispositivo disponible que no esté ya en uso
        if (usedDeviceName.empty()) {
            std::lock_guard<std::mutex> lock(available_playbacks_mtx_);
            for (std::string const& candidate : available_playbacks_) {
                bool enUso = false;
                for (auto const& [name, player] : playersMorse_) {
                    if (player->getDeviceName() == candidate) {
                        enUso = true;
                        break;
                    }
                }
                if (!enUso) {
                    usedDeviceName = candidate;
                    break;
                }
            }
        }


        // Comprobar si se ha obtenido bien el dispositivo
        if (usedDeviceName.empty()) {
            SYS_WARN("SoundMgr","Cannot retrieve device name");
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
            SYS_WARN("SoundMgr", "Failed to found device: '" + usedDeviceName + "'"); 
            return false; 
        }

        // Corrige la config con el nombre real completo del dispositivo encontrado
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing playback module: " + usedModuleName);
        std::unique_ptr<PlayerMorse> pm = std::make_unique<PlayerMorse>(
            usedModuleName,
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback module...");
        if (!pm->init(config, usedModuleName))
        {
            SYS_WARN("SoundMgr","Failed to initialize playback module");
            return false;
        }

        // Guardar en la lista de Players de Morse
        playersMorse_[usedModuleName] = std::move(pm);
        SYS_INFO("SoundMgr", "New playback module added: '" + usedModuleName + "' (" + realDeviceName + ")");
        return true;
    }

    bool SoundMgr::removePlayerMorse(std::string const& moduleName) {
       return remove_module(moduleName, playersMorse_);
    }

    PlayerMorse* SoundMgr::getPlayerMorse(std::string moduleName) const {
        auto it = playersMorse_.find(moduleName);
        return (it != playersMorse_.end() && it->second) ? it->second.get() : nullptr;
    }

    
    // Módulos PlayerTTS -------------------------------------------------------------

    bool SoundMgr::addPlayerTTS(
        void* config,
        std::string const& moduleName,
        std::string const& deviceName) 
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
        std::string usedModuleName  = moduleName;  // Nombre definitivo del módulo (del param o json)
        JsonMgr& jsonMgr = JsonMgr::instance();
        if (usedDeviceName.empty() || usedModuleName.empty()) {
            if (!config) {
                SYS_WARN("SoundMgr","Cannot retrieve module name");
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
            SYS_WARN("SoundMgr","Cannot retrieve device name");
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
            SYS_WARN("SoundMgr", "Failed to found device: '" + usedDeviceName + "'"); 
            return false; 
        }

        // Corrige la config con el nombre real completo del dispositivo encontrado
        if (config) 
            jsonMgr.set(static_cast<json*>(config), "device", realDeviceName);

        // Crear módulo de audio
        SYS_INFO("SoundMgr", "Initializing playback module: " + usedModuleName);
        std::unique_ptr<PlayerTTS> pt = std::make_unique<PlayerTTS>(
            usedModuleName,
            &pimpl_->snd_context_,
            selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback module...");
        if (!pt->init(config, usedModuleName))
        {
            SYS_WARN("SoundMgr","Failed to initialize playback module");
            return false;
        }

        // INYECCIÓN: Generar audio del texto usando TTSCore
        pt->setCallback_onTextToAudio([this](std::string const& modelName, std::string const& text) -> AudioData {
            return tts_->generate(modelName, text);
        });

        // Guardar en PlayerTTS en la lista de Players
        playersTTS_[usedModuleName] = std::move(pt);
        return true;
    }

    bool SoundMgr::removePlayerTTS(std::string const& moduleName) {
        return remove_module(moduleName, playersTTS_);
    }

    PlayerTTS* SoundMgr::getPlayerTTS(std::string moduleName) const {
        auto it = playersTTS_.find(moduleName);
        return (it != playersTTS_.end() && it->second) ? it->second.get() : nullptr;
    }

    TTSCore* SoundMgr::getTTSCore() const {
        return tts_.get();
    }

    
    // Datos de dispositivos ----------------------------------------------------------------

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

    void SoundMgr::listAvailablePlaybacks() const {
        for (std::string const& name : getAvailablePlaybacks())
            SYS_INFO("SoundMgr", "Playback: " + name);
    }
    
    std::vector<std::string> SoundMgr::getAvailableCaptures() const {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        return available_inputs_;
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

    void SoundMgr::listAvailableCaptures() const {
        for (std::string const& name : getAvailableCaptures())
            SYS_INFO("SoundMgr", "Capture: " + name);
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
        return true;
    }


    // Listas de dispositivos de audio ------------------------------------------------------

    void SoundMgr::update_available_inputs() {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        available_inputs_.clear();
        for (unsigned int i = 0; i < pimpl_->captureDevCount_; ++i) 
            available_inputs_.push_back(pimpl_->pCaptureDevInfos_[i].name);
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
void SoundMgr::listAvailableCaptures() const                                { return; }
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
void SoundMgr::update_managed_inputs()      { return; }
void SoundMgr::update_available_playbacks() { return; }
void SoundMgr::update_managed_playbacks()   { return; }

// Morse --------------------------------------------------------------------------------
std::vector<float> SoundMgr::generate_morse(std::string const&, std::string const&) const  { return {}; }

#endif
