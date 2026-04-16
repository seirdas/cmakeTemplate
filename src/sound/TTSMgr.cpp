#include "sound/TTSMgr.hpp"
#include "system/sys.hpp"
#include <vector>
#include <cstring>
#include <mutex>

// Este #define está definido en lib-sherpaonnx.cmake según la ruta de descarga
// Se redefine aquí por si acaso, pero no se debería usar ésta
#ifndef VOICES_PATH
    #define VOICES_PATH "tts-voices"
#endif

namespace fs = std::filesystem;

TTSMgr::TTSMgr(std::size_t const& num_threads_) :
    num_threads_(num_threads_ == 0 ? 1 : num_threads_),
    concurrent_init_(true),
    models_path_(VOICES_PATH),
    num_available_models_(0),
    num_loaded_models_(0),
    active_tasks_(0)
{};

TTSMgr::~TTSMgr() {
    cerrar();
};

bool TTSMgr::init() {
    // Marcar como corriendo por si se destruye entre las generaciones
    running_ = true;

    // obtener la lista de rutas de los modelos de la ruta models_path_
    std::vector<std::string> models_str = getAvailableModels();
    std::vector<std::filesystem::path> available_models(models_str.begin(), models_str.end());

    if (available_models.empty())
        SYS_WARN("TTSMgr", "Cannot read TTS voice models");

    // Inicialización concurrente (experimental)
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
        for (std::filesystem::path const& modelDir : available_models)
            load_vits_model(modelDir);
    }

    SYS_INFO(
        "TTSMgr", 
        std::to_string(num_loaded_models_) + "/" + std::to_string(num_available_models_) + " TTS models loaded."
    );

    if (num_loaded_models_ != num_available_models_) {
        SYS_WARN("TTSMgr", std::to_string(num_available_models_-num_loaded_models_) + " models left.");
        loadRemaining();
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

    // Destruye los modelos creados
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

void TTSMgr::loadRemaining() {
    // TODO
    /* la idea es cargar los que falten cuando el init "falle" con los que falle*/
}

// Datos generales ----------------------------------------------------------------------

std::vector<std::string> TTSMgr::getAvailableModels() {
    std::vector<std::string> available_models;

    if (!fs::is_directory(models_path_)) {
        SYS_WARN("TTSMgr","Path '" + models_path_ + "' not found.");
        return {};
    }

    for (const auto& entry : fs::directory_iterator(models_path_))
        if (entry.is_directory())
            available_models.push_back(fs::absolute(entry.path()).string());

    if (available_models.empty()) {
        SYS_WARN("TTSMgr","Cannot load any voice model. Check assets folder " + models_path_);
        return {};
    }

    num_available_models_ = available_models.size();
    return available_models;
}

std::vector<std::string> TTSMgr::getLoadedModels() const {
    std::vector<std::string> models;
    for(const auto& [name, model] : loaded_models_)
        models.push_back(name);
    return models;
}

short TTSMgr::getAvailableNumModels() const {
    return num_available_models_;
}

short TTSMgr::getLoadedNumModels() const {
    return num_loaded_models_;
};

bool TTSMgr::isWorking() const {
    return active_tasks_>0;
}

// Control de modelos -----------------------------------------------------------------

bool TTSMgr::generate(std::string const& modelName, std::string const& text, std::string const& wavname){
    if (!running_) return false;

    if (text == "") {
        SYS_WARN("TTSMgr", "Generate function called with empty string");
        return false;
    }

    // Como ejemplo, de momento coge el primero de la lista. hay que añadir los demás como parámetro de la función
    std::vector<std::string> models_names = getLoadedModels();
    if (models_names.empty()) {
        SYS_WARN("TTSMgr","Cannot load any model");
        return false;
    }

    if (loaded_models_.find(modelName) == loaded_models_.end()) {
        SYS_ERROR("Model not found or loaded: " + modelName, "TTSMgr");
        return false;
    }

    // Inicia el proceso
    SYS_INFO("TTSMgr","Generating audio '" + wavname +".wav'  with model " + modelName + "...");
    active_tasks_++;

    // Guardar el texto en proceso
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
    active_tasks_--;
    exit_cv_.notify_all();
    
    // Comprobar si se ha generado audio
    if (!audio) {
        SYS_WARN("TTSMgr","Cannot generate audio.");
        return false;
    }

    // Generar archivo de audio
    SYS_INFO("TTSMgr","Writing to file...");
    active_tasks_++;
    SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, (wavname+".wav").c_str());
    active_tasks_--;
    exit_cv_.notify_all();

    // Limpiar el texto del modelo
    {
        std::lock_guard<std::mutex> lock(processing_mtx_);
        processing_texts_.erase(modelName);
    }

    // Liberar memoria para evitar fugas
    SYS_INFO("TTSMgr","Freeing memory...");
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

    SYS_INFO("TTSMgr","Audio generated: " + wavname + ".wav"); 
    return true;
};

int TTSMgr::getSampleRate(std::string const& modelName) const {
    auto it = loaded_models_.find(modelName);
    if (it == loaded_models_.end()) return 0;
    return SherpaOnnxOfflineTtsSampleRate(it->second);
}

int TTSMgr::getNumSpeakers(std::string const& modelName) const {
    auto it = loaded_models_.find(modelName);
    if (it == loaded_models_.end()) {
        SYS_WARN("TTSMgr", "Cannot find '"+modelName+"' model.");
        return 0;
    }
    return SherpaOnnxOfflineTtsNumSpeakers(it->second);
}

std::string TTSMgr::getProccesingText(std::string const& modelName) const{
    std::lock_guard<std::mutex> lock(processing_mtx_);
    auto it = processing_texts_.find(modelName);
    if (it != processing_texts_.end()) {
        return it->second;
    }
    return ""; // Retorna vacío si el modelo no está procesando nada
}

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

    // Inicializa el modelo con la configuración (tarda un poco)
    SYS_INFO("TTSMgr","Initializating voice model " + st_modelname);
    active_tasks_++;
    const SherpaOnnxOfflineTts* tts_model = SherpaOnnxCreateOfflineTts(&config);
    active_tasks_--;
    exit_cv_.notify_all();

    // Comprueba si se ha generado bien
    if (!tts_model) SYS_ERROR("Cannot load voice model: " + st_modelname_path, "TTSMgr");

    // Agregarlo a la lista de modelos disponibles del TTSMgr
    std::lock_guard<std::mutex> lock(models_mutex_);
    loaded_models_[st_modelname] = tts_model;

    // Actualizar el porcentaje de inicialización
    num_loaded_models_ = loaded_models_.size();

    return true;
}