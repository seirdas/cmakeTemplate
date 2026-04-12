#include "sound/TTSMgr.hpp"
#include "system/sys.hpp"
#include <iostream>
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
    concurrent_init_(false),
    init_percent_(0),
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
    auto models_str = getAvailableModels();
    std::vector<std::filesystem::path> available_models(models_str.begin(), models_str.end());

    if (available_models.empty())
        SYS_WARN("TTSMgr", "Cannot read TTS voice models");
    else num_available_models_ = available_models.size();

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

    // Actualizar el porcentaje de inicialización (de nuevo, opcional)
    init_percent_ = static_cast<int>(100.0 * num_loaded_models_ / num_available_models_);
    return !loaded_models_.empty();;
}

void TTSMgr::cerrar() {
    if (!running_) return;
    running_ = false;

    // Esperar a que terminen las operaciones que se estaban ejecutando
    std::unique_lock<std::mutex> lock(exit_mtx_);
    std::cout << "[TTSMgr]  Waiting for unfinished jobs... " << std::endl;
    exit_cv_.wait(lock, [this] {
        return active_tasks_ == 0;
    });

    // Destruye los modelos creados
    for (auto& [name,model] : loaded_models_) {
        std::cout << "[TTSMgr]  Unloading model " << name << std::endl;
        SherpaOnnxDestroyOfflineTts(model);
    }
    loaded_models_.clear();
    init_percent_ = static_cast<int>(100.0 * num_loaded_models_ / num_available_models_);
}

void TTSMgr::reload() {
    cerrar();
    init();
}

bool TTSMgr::generate(std::string text, std::string wavname){
    if (!running_) return false;

    int sid = 0; // speaker id (voces dentro del modelo, puede tener más de una, ver en su json)

    // Prueba a generar el audio con el primer modelo cargado (TODO implementar demás modelos)
    std::cout << "[TTSMgr]  Generating audio" << std::endl;
    active_tasks_++;
    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(loaded_models_[getLoadedModels()[0]], text.c_str(), sid, 1.0);
    active_tasks_--;
    exit_cv_.notify_all();

    if (!audio) {
        std::string err = "[TTSMgr]  ERROR Cannot generate audio: " + wavname;
        std::cerr << err << std::endl;
        #ifdef _WIN32
            MessageBoxA(NULL, err.c_str(), "ERROR", MB_ICONERROR | MB_OK);
        #else
            std::string comando = "zenity --error --title=\"TTS ERROR\" --text=\"" + err + "\" 2>/dev/null";
            system(comando.c_str());
        #endif
        return false;
    }

    std::cout << "[TTSMgr]  Writing to file..." << std::endl;
    active_tasks_++;
    SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, (wavname+".wav").c_str());
    active_tasks_--;
    exit_cv_.notify_all();

    // You need to free the pointers to avoid memory leak in your app
    std::cout << "[TTSMgr]  Freeing memory" << std::endl;
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

    std::cout << "[TTSMgr]  Audio generated: " << wavname << ".wav" << std::endl; 
    
    return true;
};

short TTSMgr::getInitPercent() const {
    return init_percent_;
}

std::vector<std::string> TTSMgr::getAvailableModels() {
    std::vector<std::string> available_models;

    if (!fs::is_directory(models_path_)) {
        std::cerr << "[TTSMgr] ERROR: path '" << models_path_ << "' not found." << std::endl;
        return {};
    }

    for (const auto& entry : fs::directory_iterator(models_path_))
        if (entry.is_directory())
            available_models.push_back(fs::absolute(entry.path()).string());

    if (available_models.empty()) {
        std::cerr << "[TTSMgr]  Cannot load any voice model.";
        std::cerr << "Check assets folder " << models_path_ << std::endl;
        return {};
    }
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
    std::cout << "[TTSMgr]  Initializating voice model " << st_modelname << std::endl;
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
    init_percent_ = static_cast<int>(100.0 * getLoadedNumModels() / getAvailableNumModels());

    return true;
}