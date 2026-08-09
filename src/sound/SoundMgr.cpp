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
        smoothedValues_(true),
        attackCoeff_(0.5),
        releaseCoeff_(0.1),
        morseUnitMs_(100),
        morseSampleRate_(48000)
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

    bool SoundMgr::stop() {
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


    // Dispositivos de captura --------------------------------------------------------------

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

        // Insertar el nuevo aim inicializado en el vector de capturas
        captures_[myCaptureName] = std::move(aim);
        SYS_INFO("SoundMgr", "New input device: '" + myCaptureName + "' (" + myDeviceName + ")");

        // Actualizar la lista de capturas gestionadas
        update_managed_inputs();

        return true;
    }

    bool SoundMgr::removeCaptureDevice(std::string const& name) {

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

        // Actualizar la lista de capturas gestionadas
        SYS_INFO("SoundMgr", "Deleted Capture Device.");
        update_managed_inputs();
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

        if(it->second->isRecording()) 
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
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        return available_playbacks_;
    }

    std::vector<std::string> SoundMgr::getManagedPlaybacks() const {
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        return managed_playbacks_;
    }

    bool SoundMgr::isOnManagedPlaybacks(std::string const& playbackName) const {
        // Si no está inicializado no se puede hacer nada
        if (!initialized_) return false;

        // Recorre todos los dispositivos de playback 
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i)
            if (pimpl_->pPlaybackDevInfos_[i].name == playbackName)
                return true;

        /*else*/ return false;
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
            selectedDeviceInfo
        );

        // Intentar inicializar
        SYS_INFO("SoundMgr", "Initializing playback...");
        if (!apm->init())
        {
            // No hay nada que limpiar, el puntero make_unique se destruye al salir.
            SYS_WARN("SoundMgr","Failed to initialize playback.");
            return false;
        }

        // Insertar el nuevo apm inicializado en el vector de playbacks
        playbacks_[deviceName] = std::move(apm);
        SYS_INFO("SoundMgr", "Playback loaded. ");

        // Actualizar la lista de capturas gestionadas
        update_managed_playbacks();

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
        ultimoAPM->stop();

        // Remover el APM de la lista
        removePlaybackDevice(defDevice);

        return true;
    }


    // Listas de dispositivos de audio ------------------------------------------------------

    void SoundMgr::update_available_inputs() {
        std::lock_guard<std::mutex> lock(available_inputs_mtx_);
        available_inputs_.clear();
        for (ma_uint32 i = 0; i < pimpl_->captureDevCount_; ++i) 
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
        for (ma_uint32 i = 0; i < pimpl_->PlaybackDevCount_; ++i) 
            available_playbacks_.push_back(pimpl_->pPlaybackDevInfos_[i].name);
    }

    void SoundMgr::update_managed_playbacks() {
        std::lock_guard<std::mutex> lock(managed_playbacks_mtx_);
        managed_playbacks_.clear();
        for (auto& [name, apm] : playbacks_)
            managed_playbacks_.push_back(name);
    }
    

    // Morse ---------------------------------------------------------------------------------
 
    std::vector<float> SoundMgr::generateMorse(std::string const& tipo, std::string const& texto) const {
    
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
