#include "sound/TTSMgr.hpp"
#include "system/SystemMgr.hpp"

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

    TTSMgr::TTSMgr(std::size_t const& num_threads_) :
        num_threads_(num_threads_ == 0 ? 1 : num_threads_),
        concurrent_init_(false),
        models_path_(VOICES_PATH),
        num_available_models_(0),
        active_tasks_(0),
        num_load_retries_(2),
        running_(false),
        loading_(false)
    {

    };

    TTSMgr::~TTSMgr() {
        cerrar();
    };


    // Ejecución ----------------------------------------------------------------------------

    bool TTSMgr::init() {
        // Marcar como corriendo por si se destruye mientras carga modelos
        running_ = true;

        // Comprobar que existe el directorio de modelos de voz
        if (!fs::is_directory(models_path_)) {
            SYS_WARN("TTSMgr","Path '" + models_path_ + "' not found.");
            return {};
        }

        // obtener la lista de rutas de los modelos de la ruta models_path_
        std::vector<std::string> models_str = getAvailableModelsPath();
        std::vector<std::filesystem::path> available_models(models_str.begin(), models_str.end());

        if (available_models.empty()) {
            SYS_WARN("TTSMgr", "Cannot read TTS voice models");
            return false;
        }

        // Inicialización concurrente (experimental)
        loading_ = true;
        if (concurrent_init_) {
            // Realizar la inicialización de cada modelo en hilos independientes 
            std::vector<std::thread> workers;
            for (const auto& modelDir : available_models)
                workers.emplace_back(&TTSMgr::load_vits_model, this, modelDir);    // Un hilo por cada inicialización

            // Esperar a que todos los hilos terminen
            for (auto& t : workers) 
                if (t.joinable()) 
                    t.join();   
        }
        // Inicialización consecutiva (normal) 
        else {
            // Iterar por todas las carpetas de modelos
            for (std::filesystem::path const& modelDir : available_models) {
                // No cargar el modelo o los siguientes si se ha cerrado la app
                if (!running_) break;
                load_vits_model(modelDir);
            }
        }
        loading_ = false;

        short numLoaded     = numLoadedModels();
        short numAvailable  = numAvailableModels();

        // Salir de este init si se ha cerrado la app mientras cargaba modelos
        if (!running_) return false;

        std::string msg = std::to_string(numLoaded) + "/" + std::to_string(numAvailable) + " TTS models loaded.";
        if (numLoaded == numAvailable) {
            SYS_INFO("TTSMgr", msg);
        } else {

            // Intentar cargar los modelos faltantes si han fallado (varios intentos)
            SYS_WARN("TTSMgr", msg + " Trying to load missing models...");
            for (unsigned int i = 0; i < num_load_retries_; i++) {
                loadMissingModels();
                numLoaded     = numLoadedModels();
                if (numLoaded == numAvailable)
                    break;
            }
            if (numLoaded == numAvailable)
                SYS_SOLVED("TTSMgr","Missing TTS models loaded (" + std::to_string(numLoadedModels()) + "/" + std::to_string(numAvailableModels()) + ")" );
            else
                SYS_WARN("TTSMgr","Cannot load all models");
        }

        return !loaded_models_.empty();;
    }

    void TTSMgr::cerrar() {
        if (!running_) return;
        running_ = false;

        // Esperar a que terminen las operaciones que se estaban ejecutando
        std::unique_lock<std::mutex> lock(exit_mtx_);
        SYS_INFO("TTSMgr","Waiting for unfinished jobs... ");
        exit_cv_.wait(lock, [this] {
            return active_tasks_ == 0;
        });

        // Destruye los modelos creados (opcional con cxx, mejor)
        std::lock_guard<std::mutex> lock2(models_mutex_);
        for (auto& [name,model] : loaded_models_) {
            SYS_INFO("TTSMgr","Unloading model " + name);
            SherpaOnnxDestroyOfflineTts(model);
        }
        loaded_models_.clear();
    }

    void TTSMgr::reload() {
        cerrar();
        init();
    }

    void TTSMgr::loadMissingModels() {

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

    bool TTSMgr::isWorking() const {
        return active_tasks_>0;
    }


    // Datos de los modelos -----------------------------------------------------------------

    std::vector<std::string> TTSMgr::getAvailableModels() {

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

    std::vector<std::string> TTSMgr::getAvailableModelsPath() {

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

    std::vector<std::string> TTSMgr::getLoadedModels() const {
        std::vector<std::string> models;

        // Rellena la lista de modelos inicializados y lo devuelve
        std::lock_guard<std::mutex> lock(models_mutex_);
        for(const auto& [name, model] : loaded_models_)
            models.push_back(name);
        return models;
    }

    std::string TTSMgr::getModelPath(std::string model) const {

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

    short TTSMgr::numAvailableModels() const {
        return num_available_models_;
    }

    short TTSMgr::numLoadedModels() const {
        std::lock_guard<std::mutex> lock(models_mutex_);
        return loaded_models_.size();
    };


    // Datos y control de modelos -----------------------------------------------------------

    bool TTSMgr::generate(std::string const& modelName, std::string const& text, std::string const& wavname) {
        if (!running_) return false;

        // Comprobar que el texto contiene algo para generar
        if (text == "") {
            SYS_WARN("TTSMgr", "Generate function called with empty string");
            return false;
        }

        // Comprobar los modelos disponibles
        std::vector<std::string> models_names = getLoadedModels();
        if (models_names.empty()) {
            SYS_WARN("TTSMgr","Cannot load any model");
            return false;
        }

        // Comprobar que el modelo "seleccionado" por param existe
        {
            std::lock_guard<std::mutex> lock(models_mutex_);
            auto it = loaded_models_.find(modelName);
            if (it == loaded_models_.end()) {
                SYS_ERROR("Model not found or loaded: " + modelName, "TTSMgr");
                return false;
            }
        }

        // Inicia el proceso
        SYS_INFO("TTSMgr","Generating audio '" + wavname +".wav'  with model " + modelName + "...");
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

        // Notifica al terminar de generar
        exit_cv_.notify_all();
        
        // Comprobar si se ha generado audio
        if (!audio) {
            SYS_WARN("TTSMgr","Cannot generate audio.");
            return false;
        }

        // Generar archivo de audio
        SYS_INFO("TTSMgr","Writing to file...");
        SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, (wavname+".wav").c_str());
        active_tasks_--;
        exit_cv_.notify_all();

        // Limpiar el texto que está generando el modelo (ya ha terminado de generar)
        {
            std::lock_guard<std::mutex> lock(processing_mtx_);
            processing_texts_.erase(modelName);
        }

        // Liberar memoria para evitar fugas
        SYS_INFO("TTSMgr","Freeing memory...");
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

        // Mensaje de info
        SYS_INFO("TTSMgr","Audio generated: " + wavname + ".wav"); 
        return true;
    };

    int TTSMgr::getSampleRate(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it == loaded_models_.end()) {
            SYS_WARN("TTSMgr","Cannot get sample rate: Model '" + modelName + "' not found.");
            return 0;
        }
        
        return SherpaOnnxOfflineTtsSampleRate(it->second);
    }

    int TTSMgr::getNumSpeakers(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it == loaded_models_.end()) {
            SYS_WARN("TTSMgr","Cannot get num speakers: Model '" + modelName + "' not found.");
            return 0;
        }

        return SherpaOnnxOfflineTtsNumSpeakers(it->second);
    }

    std::string TTSMgr::getProccesingText(std::string const& modelName) const {

        // Busca el modelo, avisa si no lo encuentra
        std::lock_guard<std::mutex> lock(processing_mtx_);
        auto it = processing_texts_.find(modelName);
        if (it == processing_texts_.end()) {
            SYS_WARN("TTSMgr","Cannot get proccesing text: Model '" + modelName + "' not found.");
            return "";
        }

        return it->second;
    }


    // Inicialización de modelos ------------------------------------------------------------

    bool TTSMgr::load_vits_model(std::filesystem::path modelDir) {
        // No inicializar si se está cerrando
        if (!running_) 
            return false;

        // Rutas
        std::string st_modelname_path, st_tokens_path, st_datadir_path, st_modelname;

        // Iterar por los elementos de la carpeta
        for (const auto& file : fs::directory_iterator(modelDir)) {
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
        SYS_INFO("TTSMgr","Initializating voice model " + st_modelname);
        active_tasks_++;
        const SherpaOnnxOfflineTts* tts_model = SherpaOnnxCreateOfflineTts(&config);
        active_tasks_--;
        exit_cv_.notify_all();

        // Comprueba si se ha generado bien
        if (!tts_model) SYS_ERROR("TTSMgr","Cannot load voice model: " + st_modelname_path);

        // Agregarlo a la lista de modelos disponibles del TTSMgr
        std::lock_guard<std::mutex> lock(models_mutex_);
        loaded_models_[st_modelname] = tts_model;

        // Todo correcto
        return true;
    }

    bool TTSMgr::unload_model(std::string const& modelName) {

        // Busca el modelo y lo borra
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(modelName);
        if (it != loaded_models_.end()) {
            SherpaOnnxDestroyOfflineTts(it->second);
            loaded_models_.erase(it);
            SYS_INFO("TTSMgr", "Model '" + modelName + "' unloaded successfully");
            return true;
        }
        
        /* else */  // No ha encontrado el modelo
        SYS_WARN("TTSMgr", "Cannot unload model: Model '" + modelName + "' not found");
        return false;
    }
    

#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    TTSMgr::TTSMgr(std::size_t const&) {
		SYS_WARN("TTSMgr", "Sherpa TTS library has not been implemented.");
    };
    TTSMgr::~TTSMgr() {};

    // Ejecución ----------------------------------------------------------------------------
    bool TTSMgr::init()              { return false; };
    void TTSMgr::cerrar()            { return; };
    void TTSMgr::reload()            { return; }
    void TTSMgr::loadMissingModels() { return; }
    bool TTSMgr::isWorking() const   { return false; }

    // Datos del módulo TTS -----------------------------------------------------------------
    std::vector<std::string> TTSMgr::getAvailableModels()       { return {}; }
    std::vector<std::string> TTSMgr::getLoadedModels() const    { return {}; }
    std::string TTSMgr::getModelPath(std::string) const         { return ""; }
    short   TTSMgr::numAvailableModels() const                  { return 0; }
    short   TTSMgr::numLoadedModels() const                     { return 0; }

    // Datos y control de modelos -----------------------------------------------------------
    bool    TTSMgr::generate(std::string const&, std::string const&, std::string const&) { return false; }
    int     TTSMgr::getSampleRate(std::string const&) const         { return 0; }
    int     TTSMgr::getNumSpeakers(std::string const&) const        { return 0; }
    std::string TTSMgr::getProccesingText(std::string const&) const { return ""; }

    // Inicialización de modelos ------------------------------------------------------------
    bool TTSMgr::load_vits_model(std::filesystem::path)          { return false; }
    bool TTSMgr::unload_model(std::string const&)                { return false; }
#endif