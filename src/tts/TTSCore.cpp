#include "tts/TTSCore.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"

// Macro de cmake al activar la librería
#if defined SHERPA || defined SHERPA_VERSION

    #include "sherpa-onnx/c-api/cxx-api.h"
    #include <vector>
    #include <cstring>
    #include <mutex>
    #include <algorithm> // Para std::sort y std::set_difference
    #include <iterator>  // Para std::back_inserter

    // Este #define está definido en lib-sherpaonnx.cmake según la ruta de descarga
    // Se redefine aquí por si acaso, pero no se debería usar ésta
    #ifndef VOICES_PATH
        #define VOICES_PATH "tts-voices"
    #endif

    namespace fs = std::filesystem;

    // General ------------------------------------------------------------------------------

    TTSCore::TTSCore(std::size_t const& num_threads_) :
        num_threads_(num_threads_ == 0 ? 1 : num_threads_),
        concurrent_init_(false),
        lazy_load_(true),
        keep_alive_seconds_(20),
        models_path_(VOICES_PATH),
        num_available_models_(0),
        active_tasks_(0),
        num_load_retries_(2),
        running_(false),
        loading_(false)
    {

    };

    TTSCore::~TTSCore() {
        close();
    };


    // Ejecución ----------------------------------------------------------------------------

    bool TTSCore::init(void* config) {
        
        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else // Puede llegar aquí si se hace reload()
            SYS_WARN("TTSCore","Cannot load config. Using default values.");

        // Marcar como corriendo por si se destruye mientras carga modelos
        running_ = true;

        // Comprobar que existe el directorio de modelos de voz
        if (!fs::is_directory(models_path_)) {
            SYS_WARN("TTSCore","Path '" + models_path_ + "' not found.");
            return {};
        }

        // Carga los modelos
        if (!lazy_load_)
            loadModels();
        else {
            // Con lazy_load, marcar el modelo como cargado si está disponible

            // obtener la lista de rutas de los modelos de la ruta models_path_
            std::vector<std::string> av_models_str = getAvailableModelsPath();
            std::vector<std::filesystem::path> available_models(av_models_str.begin(), av_models_str.end());

            if (available_models.empty()) {
                SYS_WARN("TTSCore", "Cannot read any TTS voice models");
                return false;
            }

            // Iterar por todas las carpetas de modelos para marcar modelo como "cargado"
            std::lock_guard<std::mutex> lock(models_mutex_);
            for (std::filesystem::path const& modelDir : available_models) {
                if (!running_) break;                               // Salir si el tts no está corriendo (cierre prematuro)
                if (!checkAvailableModel(modelDir)) continue;       // No crear entrada si faltan archivos
                loaded_models_[getModelName(modelDir)] = nullptr;   // rellenar el vector con puntero nulo
            }
        }

        short numLoaded     = (lazy_load_) ? getLoadedModels().size() : numLoadedModels();  // con lazy_load, numLoaded es = 0
        short numAvailable  = numAvailableModels();

        // Salir de este init si se ha cerrado la app mientras cargaba modelos
        if (!running_) return false;

        // Mensaje de info: finalizando init
        std::string msg = std::to_string(numLoaded) + "/" + std::to_string(numAvailable) + " TTS models loaded";
        if (lazy_load_) msg += (" (lazy load enabled)");
        if (numLoaded == numAvailable || lazy_load_) {
            SYS_INFO("TTSCore", msg);
        } else {
            // Intentar cargar los modelos faltantes si han fallado (varios intentos)
            SYS_WARN("TTSCore", msg + " Trying to load missing models...");
            for (unsigned int i = 0; i < num_load_retries_; i++) {
                loadMissingModels();
                numLoaded     = numLoadedModels();
                if (numLoaded == numAvailable)
                    break;
            }
            if (numLoaded == numAvailable)
                SYS_SOLVED("TTSCore","Missing TTS models loaded (" + std::to_string(numLoadedModels()) + "/" + std::to_string(numAvailableModels()) + ")" );
            else
                SYS_WARN("TTSCore","Cannot load all models");
        }

        // Si está lazy_load activo, activar el thread reaper
        if (lazy_load_ && keep_alive_seconds_.count() > 0)
            keepalive_thread_ = std::thread(&TTSCore::keepAliveWorker, this);

        return !loaded_models_.empty();;
    }

    bool TTSCore::isInitialized() const{
        return initialized_;
    }
    
    void TTSCore::loadConfig(void* config) {

        if (!config)
            return;

        SYS_INFO("TTSCore","Reading config node...");

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Carga de la sección lazy
        json* lazyCfg = jsonMgr.getSubNode(cfg, "lazy_load");
        jsonMgr.get_or_set(lazyCfg, "active", lazy_load_);

        int keep_alive_raw = static_cast<int>(keep_alive_seconds_.count()); // numero a segundos
        jsonMgr.get_or_set(lazyCfg, "keep_alive_seconds", keep_alive_raw);
        keep_alive_seconds_ = std::chrono::seconds(keep_alive_raw);

        // Carga de la sección concurrent
        json* concurrentCfg = jsonMgr.getSubNode(cfg, "concurrent_init_");
        jsonMgr.get_or_set(concurrentCfg, "active", concurrent_init_);
        jsonMgr.get_or_set(concurrentCfg, "num_load_retries", num_load_retries_);

        SYS_INFO("TTSCore","Config node read OK");
    }

    void TTSCore::close() {
        if (!running_) return;
        running_ = false;

        // lazy_load, parar y unir el hilo reaper antes de tocar/destruir los modelos
        keepalive_cv_.notify_all();
        if (keepalive_thread_.joinable())
            keepalive_thread_.join();

        // Esperar a que terminen las operaciones que se estaban ejecutando
        std::unique_lock<std::mutex> lock(exit_mtx_);
        SYS_INFO("TTSCore","Waiting for unfinished jobs... ");
        exit_cv_.wait(lock, [this] {
            return active_tasks_ == 0;
        });

        // Destruye los modelos creados (opcional con cxx, mejor)
        std::lock_guard<std::mutex> lock2(models_mutex_);
        for (auto& [name,model] : loaded_models_) {
            if (!model) continue;
            SYS_INFO("TTSCore","Unloading model " + name);
            SherpaOnnxDestroyOfflineTts(model);
        }
        loaded_models_.clear();
    }

    void TTSCore::reload() {
        close();
        init(nullptr);
    }

    void TTSCore::loadMissingModels() {

        // No cargar modelos restantes si se está cerrrando
        if(!running_) return;

        // No cargar modelos si se están cargando por otro lado
        if(loading_) return;

        // No hacer nada si no hay modelos disponibles
        if(num_available_models_==0) return;

        // Variables locales:
        std::vector<std::string> av_models_path_str;        // Lista de rutas de modelos disponibles
        std::vector<std::string> load_models_str;           // Lista de modelos cargados
        std::vector<std::string> load_models_path_str;      // Lista de rutas de modelos cargados
        std::vector<std::string> missing_models_path_str;   // Lista de rutas de modelos restantes

        // Obtener la ruta de los modelos disponibles
        av_models_path_str = getAvailableModelsPath();
        
        // Obtener la ruta de los modelos cargados para ver posteriormente la diferencia
        load_models_str = getLoadedModels();
        for (std::string const& model : load_models_str) 
            load_models_path_str.push_back(getModelPath(model));

        // Ordenar ambos vectores de rutas (Obligatorio para set_difference)
        std::sort(av_models_path_str.begin(), av_models_path_str.end());
        std::sort(load_models_path_str.begin(), load_models_path_str.end());

        // Obtener la "diferencia", las rutas de modelos disponibles que no están cargados
        std::set_difference(
            av_models_path_str.begin(), av_models_path_str.end(),
            load_models_path_str.begin(), load_models_path_str.end(),
            std::back_inserter(missing_models_path_str)
        );

        // Inicializar los modelos disponibles faltantes obtenidos (secuencial forzado, sin hilos)
        loading_ = true;
        for (std::string const& model_path : missing_models_path_str)
            load_vits_model(model_path);
        loading_ = false;

        return;
    }

    bool TTSCore::isWorking() const {
        return active_tasks_>0;
    }


    // Datos de modelos disponibles/cargados ------------------------------------------------

    std::vector<std::string> TTSCore::getAvailableModels() {

        // Comprobar que existe el directorio de modelos de voz
        if (!fs::is_directory(models_path_))
            return {};

        // Rellena el vector con el nombre de los modelos (nombre de carpeta)
        std::vector<std::string> available_models;
        for (const auto& entry : fs::directory_iterator(models_path_)) 
            if (entry.is_directory()) 
                available_models.push_back(entry.path().filename().string());

        // Guarda internamente el número de modelos
        num_available_models_ = static_cast<short>(available_models.size());

        // Devuelve la lista de nombres de los modelos
        return available_models;
    }

    std::vector<std::string> TTSCore::getAvailableModelsPath() {

        // Comprobar que existe el directorio de modelos de voz
        if (!fs::is_directory(models_path_)) 
            return {};

        // Rellena el vector con el nombre de los modelos (ruta de carpeta)
        std::vector<std::string> available_models;
        for (const auto& entry : fs::directory_iterator(models_path_))
            if (entry.is_directory())
                available_models.push_back(fs::absolute(entry.path()).string());

        // Guarda internamente el número de modelos
        num_available_models_ = static_cast<short>(available_models.size());

        // Devuelve la lista de nombres de los modelos
        return available_models;
    }

    std::vector<std::string> TTSCore::getLoadedModels() const {
        std::vector<std::string> models;

        // Rellena la lista de modelos inicializados y lo devuelve
        std::lock_guard<std::mutex> lock(models_mutex_);
        for(const auto& [name, model] : loaded_models_)
            models.push_back(name);
        return models;
    }

    std::string TTSCore::getModelPath(std::string model) const {

        // Comprobar que existe el directorio de modelos de voz
        if (!fs::is_directory(models_path_))
            return {};

        // Busca y devuelve la ruta del modelo (carpeta) si existe en ese directorio
        for (const auto& entry : fs::directory_iterator(models_path_))
            if (entry.is_directory() && entry.path().filename().string().find(model) != std::string::npos)
                return fs::absolute(entry.path()).string();

        /* else */
        return {};
    }

    short TTSCore::numAvailableModels() const {
        return num_available_models_;
    }

    short TTSCore::numLoadedModels() const {
        std::lock_guard<std::mutex> lock(models_mutex_);

        // en lazy_load, solo devolver el número de los modelos cargados de verdad
        if (lazy_load_) {
            unsigned short num = 0;
            for(const auto& [name, model] : loaded_models_)
                if (model) 
                    num++;
            return num;
        }
        else return loaded_models_.size();
    };

    bool TTSCore::isModelLoaded(std::string const& modelName) const {
        std::unique_lock<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it == loaded_models_.end()) 
            return false;
        else return true;
    }

    
    // Control de modelos individuales ------------------------------------------------------

    AudioData TTSCore::generate(std::string const& modelName, std::string const& text) {
        if (!running_) return {};

        // Comprobar que el texto contiene algo para generar
        if (text == "") {
            SYS_WARN("TTSCore", "Generate function called with empty string");
            return {};
        }

        // Comprobar los modelos disponibles
        std::vector<std::string> models_names = getLoadedModels();
        if (models_names.empty()) {
            SYS_WARN("TTSCore","Cannot load any model");
            return {};
        }

        // Comprobar que el modelo "seleccionado" por param existe
        {
            std::unique_lock<std::mutex> lock(models_mutex_);
            auto it = loaded_models_.find(modelName);
            if (it == loaded_models_.end()) {
                SYS_WARN("TTSCore", "generate: Model not found or loaded: " + modelName);
                return {};
            }
            
            // (lazy_load) Si el modelo no está cargado, cargarlo ahora
            if (lazy_load_ && it->second == nullptr) {
                std::filesystem::path modpath = getModelPath(modelName);
                if (!modpath.empty()) {

                    // Se va a bloquear 'loaded_models' otra vez en la función load_vits_model
                    lock.unlock();
                    
                    if(!load_vits_model(modpath)) {
                        SYS_WARN("TTSCore","Cannot generate: Model '" + modelName + "' couldn't be loaded.");
                        return {};
                    }

                    // NUEVO: Comprobación de funciones para reconocer el idioma
                    else {
                        if(isEnglish(modelName))
                            SYS_INFO("TTSCore","English model loaded: '" + modelName + "'");
                        else if(isBritainEnglish(modelName)) {
                            SYS_INFO("TTSCore","Britain English model loaded: '" + modelName + "'");
                        }
                        else if(isSpanish(modelName))
                            SYS_INFO("TTSCore","Spanish model loaded: '" + modelName + "'");
                    }
                    
                }
            }
        }

        // Inicia el proceso
        SYS_INFO("TTSCore","Generating audio with model " + modelName + "...");
        active_tasks_++;

        // Guardar el texto en proceso por el modelo
        {
            std::lock_guard<std::mutex> lock(processing_mtx_);
            processing_texts_[modelName] = text;
        }

        // Generar configuración
        SherpaOnnxGenerationConfig config;
        std::memset(&config, 0, sizeof(config));
        config.sid = 0;                 // Speaker ID for multi-speaker models.
        config.silence_scale = 1.0f;    // Silence scale between sentences
        config.speed = 1.0f;            // Speech rate. Used only by models that support it

        // Prueba a generar el audio con el modelo cargado
        const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerateWithConfig(
            loaded_models_[modelName],
            text.c_str(), 
            &config, 
            nullptr,        // Se podría implementar un callback que vaya mostrando el progreso (hay que diferenciar cada modelo)
            nullptr
        );

        // Limpiar el texto que está generando el modelo
        {
            std::lock_guard<std::mutex> lock(processing_mtx_);
            processing_texts_.erase(modelName);
        }

        // Termina la generación
        active_tasks_--;

        // Comprobar que se ha generado audio correctamente
        if (!audio) {
            SYS_WARN("TTSCore", "SherpaOnnx failed to generate audio.");
            exit_cv_.notify_all();
            return {};
        }

        // Para lazy_load, resetea el tiempo de vida del modelo
        if(lazy_load_) {
            std::lock_guard<std::mutex> klock(keepalive_mtx_);
            last_used_[modelName] = std::chrono::steady_clock::now();
        }

        // Notifica al terminar de generar
        exit_cv_.notify_all();

        // Retornar el audio generado copiando samples y sample_rate a un AudioData
        AudioData out_audio;
        out_audio.samples.assign(audio->samples, audio->samples + audio->n);
        out_audio.sample_rate = audio->sample_rate;

        // Liberar el audio (ya guardado en AudioData)
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

        return out_audio;
    };

    bool TTSCore::generateWav(std::string const& modelName, std::string const& text, std::string wavname) {
        
        // Generar audio
        AudioData audio = generate(modelName, text);

        // Comprobar si se ha generado audio
        if (audio.empty()) {
            SYS_WARN("TTSCore", "Cannot generate audio for WAV file: Empty audio generated");
            return false;
        }

        // Generar archivo de audio
        SYS_INFO("TTSCore","Writing to file...");
        SherpaOnnxWriteWave(
            audio.samples.data(), 
            static_cast<int32_t>(audio.samples.size()), 
            audio.sample_rate, 
            (wavname + ".wav").c_str()
        );

        // Mensaje de info
        SYS_INFO("TTSCore","Audio generated: " + wavname + ".wav"); 
        return true;
    }

    int TTSCore::getSampleRate(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it == loaded_models_.end()) {
            SYS_WARN("TTSCore","Cannot get sample rate: Model '" + modelName + "' not found.");
            return 0;
        }
        
        return SherpaOnnxOfflineTtsSampleRate(it->second);
    }

    int TTSCore::getNumSpeakers(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it == loaded_models_.end()) {
            SYS_WARN("TTSCore","Cannot get num speakers: Model '" + modelName + "' not found.");
            return 0;
        }

        return SherpaOnnxOfflineTtsNumSpeakers(it->second);
    }

    std::string TTSCore::getProccesingText(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(processing_mtx_);
        auto it = processing_texts_.find(modelName);
        if (it == processing_texts_.end()) {
            SYS_WARN("TTSCore","Cannot get proccesing text: Model '" + modelName + "' not found.");
            return "";
        }

        return it->second;
    }

    std::string TTSCore::getModelName(std::filesystem::path modelAbsPath) const {
        // Rutas
        std::string st_modelname = "";

        // Iterar por los elementos de la carpeta
        for (const auto& file : fs::directory_iterator(modelAbsPath)) {
            fs::path p = file.path();
            if (p.extension() == ".onnx") 
                st_modelname = p.stem().string();   // Nombre del modelo
        }

        // Devuelve vacío si no ha encontrado el modelo
        return st_modelname;
    }
    
    
    // Idiomas-------------------------------------------------------------------------------

    bool TTSCore::isEnglish(std::string const& modelName) const{ 
        // Comprobar si el nombre del modelo (existe) contiene "en_US"
        return modelName.find("en_US") != std::string::npos;
    }

    bool TTSCore::isSpanish(std::string const& modelName) const{
        // Comprobar si el nombre del modelo (existe) contiene "es_"
        return modelName.find("es_") != std::string::npos; 
    }

    bool TTSCore::isBritainEnglish(std::string const& modelName) const{
        // Comprobar si el nombre del modelo (existe) contiene "en_GB"
        return modelName.find("en_GB") != std::string::npos; 
    }

        std::vector<std::string> TTSCore::getLoadedModelsEnglish() const {
        std::vector<std::string> models;

        // Rellena la lista solo con los modelos cargados que son inglés (US)
        std::lock_guard<std::mutex> lock(models_mutex_);
        for(const auto& [name, model] : loaded_models_)
            if (isEnglish(name))
                models.push_back(name);
        return models;
    }

    std::vector<std::string> TTSCore::getLoadedModelsSpanish() const {
        std::vector<std::string> models;

        // Rellena la lista solo con los modelos cargados que son español
        std::lock_guard<std::mutex> lock(models_mutex_);
        for(const auto& [name, model] : loaded_models_)
            if (isSpanish(name))
                models.push_back(name);
        return models;
    }

    std::vector<std::string> TTSCore::getLoadedModelsBritainEnglish() const {
        std::vector<std::string> models;

        // Rellena la lista solo con los modelos cargados que son inglés británico
        std::lock_guard<std::mutex> lock(models_mutex_);
        for(const auto& [name, model] : loaded_models_)
            if (isBritainEnglish(name))
                models.push_back(name);
        return models;
    }

    // Inicialización de modelos ------------------------------------------------------------

    void TTSCore::loadModels() {

        // obtener la lista de rutas de los modelos de la ruta models_path_
        std::vector<std::string> av_models_str = getAvailableModelsPath();
        std::vector<std::filesystem::path> available_models(av_models_str.begin(), av_models_str.end());

        if (available_models.empty()) {
            SYS_WARN("TTSCore", "Cannot read any TTS voice models");
            return;
        }

        // Inicialización concurrente (experimental)
        loading_ = true;
        if (concurrent_init_) {
            // Realizar la inicialización de cada modelo en hilos independientes 
            std::vector<std::thread> workers;
            for (const auto& modelDir : available_models)
                workers.emplace_back(&TTSCore::load_vits_model, this, modelDir);    // Un hilo por cada inicialización

            // Esperar a que todos los hilos terminen
            for (auto& t : workers) 
                if (t.joinable()) 
                    t.join();   
        }
        // Inicialización consecutiva (normal) 
        else {
            // Iterar por todas las carpetas de modelos
            for (std::filesystem::path const& modelDir : available_models) {
                if (!running_) break;
                load_vits_model(modelDir);
            }
        }
        loading_ = false;
    }

    bool TTSCore::load_vits_model(std::filesystem::path modelAbsPath) {
        // No inicializar si se está cerrando
        if (!running_) 
            return false;

        // Rutas
        std::string st_modelname_path, st_tokens_path, st_datadir_path, st_modelname;

        // Iterar por los elementos de la carpeta
        for (const auto& file : fs::directory_iterator(modelAbsPath)) {
            fs::path p = file.path();
            if (p.extension() == ".onnx") {
                st_modelname_path = p.string();     // Ruta al onnx
                st_modelname = p.stem().string();   // Nombre del modelo
            }
            else if (p.filename() == "tokens.txt")
                st_tokens_path = p.string();        // Ruta al archivo tokens.txt
            else if (p.filename() == "espeak-ng-data" && file.is_directory())
                st_datadir_path = p.string();       // Ruta a la carpeta espeak-ng-data
        }

        // Si no se completan los tres campos, no sigue
        if (st_modelname_path.empty() || st_tokens_path.empty() || st_datadir_path.empty()) 
            return false;

        // Configuración de offline tts
        SherpaOnnxOfflineTtsConfig config;
        std::memset(&config, 0, sizeof(config));
        config.model.vits.model     = st_modelname_path.c_str();
        config.model.vits.tokens    = st_tokens_path.c_str();
        config.model.vits.data_dir  = st_datadir_path.c_str();
        config.model.num_threads    = num_threads_; // CUIDADO con la generación paralela (usar varios tts a la vez)
        config.model.debug          = 0;            // 1 para logs en consola
        config.model.vits.noise_scale   = 1.0f;     // Controla la expresividad/varianza
        config.model.vits.noise_scale_w = 0.8f;     // Varianza en la duración de los fonemas
        config.model.vits.length_scale  = 1.0f;     // 1.0 = normal, >1.0 más lento, <1.0 más rápido

        // Proveedor CPU / GPU
        #ifdef _WIN32
            config.model.provider = "directml";         // Soporte para generación con GPU (Windows)
        #endif  // Fallback a CPU en caso contrario

        // Inicializa el modelo con la configuración (tarda un poco)
        SYS_INFO("TTSCore","Initializating voice model " + st_modelname);
        active_tasks_++;
        const SherpaOnnxOfflineTts* tts_model = SherpaOnnxCreateOfflineTts(&config);
        active_tasks_--;
        exit_cv_.notify_all();

        // Comprueba si se ha generado bien
        if (!tts_model) {
            SYS_WARN("TTSCore","Cannot load voice model: " + st_modelname_path);
            return false;
        }

        // Agregarlo a la lista de modelos disponibles del TTSCore
        std::lock_guard<std::mutex> lock(models_mutex_);
        loaded_models_[st_modelname] = tts_model;

        // Activar el tiempo de vida del modelo
        if (lazy_load_) {
            std::lock_guard<std::mutex> klock(keepalive_mtx_);
            last_used_[st_modelname] = std::chrono::steady_clock::now();
        }

        // Todo correcto
        return true;
    }

    bool TTSCore::checkAvailableModel(std::filesystem::path modelAbsPath) const {
        // Rutas
        std::string st_modelname_path, st_tokens_path, st_datadir_path, st_modelname;

        // Iterar por los elementos de la carpeta
        for (const auto& file : fs::directory_iterator(modelAbsPath)) {
            fs::path p = file.path();
            if (p.extension() == ".onnx") {
                st_modelname_path = p.string();     // Ruta al onnx
                st_modelname = p.stem().string();   // Nombre del modelo
            }
            else if (p.filename() == "tokens.txt")
                st_tokens_path = p.string();        // Ruta al archivo tokens.txt
            else if (p.filename() == "espeak-ng-data" && file.is_directory())
                st_datadir_path = p.string();       // Ruta a la carpeta espeak-ng-data
        }

        // Si no se completan los tres campos, no sigue
        if (st_modelname_path.empty() || st_tokens_path.empty() || st_datadir_path.empty()) 
            return false;

        /* else */
        return true;
    }

    bool TTSCore::unload_model(std::string const& modelName) {

        // Busca el modelo y lo borra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);

        // Comprobar si el modelo existe
        if (it == loaded_models_.end()) {
            SYS_WARN("TTSCore", "Cannot unload model: Model '" + modelName + "' not found");
            return false;
        }

        // Comprobar si el modelo no es nulo
        if (!it->second)  
            return true;

        // Descargar el modelo de la memoria
        SherpaOnnxDestroyOfflineTts(it->second);

        if (!lazy_load_)    // Borrar el modelo de la lista si no lazy_load
            loaded_models_.erase(it);
        else                // Si lazy_load, solo ponerlo como puntero nulo
            it->second = nullptr;

        SYS_INFO("TTSCore","Model '" + modelName + "' unloaded");

        return true;
    }

    void TTSCore::keepAliveWorker() {
        std::unique_lock<std::mutex> lock(keepalive_mtx_);

        while (running_) {

            // Tiempo de comprobación, cada tiempoVida/10 con mínimo de 1seg
            auto poll_interval = std::max(std::chrono::seconds(1), keep_alive_seconds_ / 10);

            // Espera hasta: cierre, haya algo que vigilar
            keepalive_cv_.wait_for(lock, poll_interval, [this] {
                return !running_ || !last_used_.empty();
            });
            if (!running_) break;
            if (last_used_.empty()) continue; // nada que vigilar todavía, siguiente iteración

            // Compara el tiempo de los modelos con el actual
            auto now = std::chrono::steady_clock::now();
            std::vector<std::string> candidatos;
            for (auto const& [name, last_use] : last_used_)
                if (now - last_use >= keep_alive_seconds_)
                    candidatos.push_back(name);

            // Soltar keepalive_mtx_ antes de tocar processing_mtx_/models_mutex_
            lock.unlock();

            // Descargar modelo si no está generando nada
            std::vector<std::string> modelos_a_descargar;
            for (auto const& name : candidatos) {
                bool busy;
                {
                    std::lock_guard<std::mutex> plock(processing_mtx_);
                    busy = processing_texts_.count(name) > 0;
                }
                if (!busy && unload_model(name))        //<- Aquí descarga el modelo
                    modelos_a_descargar.push_back(name);
            }

            // Volver a bloquear keepalive_mtx_ para la siguiente iteración
            lock.lock();

            // Borrar los modelos de la lista de control de tiempos de modelos
            for (auto const& name : modelos_a_descargar)
                last_used_.erase(name);
        }
    }




#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    TTSCore::TTSCore(std::size_t const&) {
		SYS_WARN("TTSCore", "Sherpa TTS library has not been implemented.");
    };
    TTSCore::~TTSCore() {};

    // Ejecución ----------------------------------------------------------------------------
    bool TTSCore::init()              { return false; };
    void TTSCore::close()            { return; };
    void TTSCore::reload()            { return; }
    void TTSCore::loadMissingModels() { return; }
    bool TTSCore::isWorking() const   { return false; }

    // Datos del módulo TTS -----------------------------------------------------------------
    std::vector<std::string> TTSCore::getAvailableModels()       { return {}; }
    std::vector<std::string> TTSCore::getAvailableModelsPath()   { return {}; }
    std::vector<std::string> TTSCore::getLoadedModels() const    { return {}; }
    std::string TTSCore::getModelPath(std::string) const         { return ""; }
    short   TTSCore::numAvailableModels() const                  { return 0; }
    short   TTSCore::numLoadedModels() const                     { return 0; }

    // Datos y control de modelos -----------------------------------------------------------
    bool    TTSCore::generate(std::string const&, std::string const&, std::string const&) { return false; }
    int     TTSCore::getSampleRate(std::string const&) const         { return 0; }
    int     TTSCore::getNumSpeakers(std::string const&) const        { return 0; }
    std::string TTSCore::getProccesingText(std::string const&) const { return ""; }
    std::string TTSCore::getModelName(std::filesystem::path)         { return ""; }

    // Inicialización de modelos ------------------------------------------------------------
    void TTSCore::loadModels()                                    { return;       }
    bool TTSCore::load_vits_model(std::filesystem::path)          { return false; }
    bool TTSCore::checkAvailableModel(std::filesystem::path) const{ return false; }
    bool TTSCore::unload_model(std::string const&)                { return false; }
    void TTSCore::keepAliveWorker()                               { return;       }
#endif