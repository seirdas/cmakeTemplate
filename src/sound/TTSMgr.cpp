#include "sound/TTSMgr.hpp"
#include <filesystem>
#include <vector>
#include <cstring>
#include <vector>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
#endif

// Este #define está definido en lib-sherpaonnx.cmake según la ruta de descarga
// Se redefine aquí por si acaso, pero no se debería usar ésta
#ifndef VOICES_PATH
    #define VOICES_PATH "tts-voices"
#endif

namespace fs = std::filesystem;

TTSMgr::TTSMgr(std::size_t const& num_threads_) :
    num_threads_(num_threads_ == 0 ? 1 : num_threads_),
    init_percent_(0),
    models_path_(VOICES_PATH)
{};

TTSMgr::~TTSMgr() {
    cerrar();
};

bool TTSMgr::init() {

    // obtener la lista de rutas de los modelos de la ruta models_path_
    std::vector<std::filesystem::path> available_models;
    if (!fs::exists(models_path_) || !fs::is_directory(models_path_)) {
        std::cerr << "[TTSMgr] ERROR: path '" << models_path_ << "' not found." << std::endl;
        return false;
    }
    for (const auto& entry : fs::directory_iterator(models_path_))
        available_models.push_back(fs::absolute(entry.path())); 
    if (available_models.empty()) {
        std::cerr << "[TTSMgr]  Cannot load any voice model.";
        std::cerr << "Check assets folder " << models_path_ << std::endl;
        return false;
    }

    // Variable para no realizar la función cada vez que se use
    size_t num_available_models = available_models.size();

    // Preparar hilos y mutex para generación paralela
    std::vector<std::thread> workers;
    std::mutex tts_init_mutex; // Para proteger tts_models_ e init_percent_

    // Iterar por todas las carpetas de modelos
    for (const auto& modelDir : available_models) {

        // Lanzamos un hilo para cargar cada modelo
        workers.emplace_back([this, modelDir, num_available_models, &tts_init_mutex]() 
            {
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
                if (st_modelname_path.empty() || st_tokens_path.empty() || st_datadir_path.empty()) return;
                
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
                const SherpaOnnxOfflineTts* tts_model = SherpaOnnxCreateOfflineTts(&config);

                // Comprueba si se ha generado bien
                if (!tts_model) {
                    std::string err = "[TTSMgr]  ERROR Cannot load voice model: " + st_modelname_path;
                    std::cerr << err << std::endl;
                    #ifdef _WIN32
                        MessageBoxA(NULL, err.c_str(), "ERROR", MB_ICONERROR | MB_OK);
                    #else
                        std::string comando = "zenity --error --title=\"TTS ERROR\" --text=\"" + err + "\" 2>/dev/null";
                        system(comando.c_str());
                    #endif
                }
                
                // Agregarlo a la lista de modelos disponibles del TTSMgr
                std::lock_guard<std::mutex> lock(tts_init_mutex);    // protege mapa y init_percent_
                tts_models_[st_modelname] = tts_model;

                // Actualizar el porcentaje de inicialización
                init_percent_ = static_cast<int>(100.0 * tts_models_.size() / num_available_models);
            }
        );
    }

    // Esperar a que todos los hilos terminen
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    std::cout << "[TTSMgr]  " << tts_models_.size() << "/" << num_available_models;
    std::cout << " TTS models loaded." << std::endl;
    return true;
}

void TTSMgr::cerrar() {
    // Destruye los modelos creados
    for (auto& [name,model] : tts_models_) {
        std::cout << "[TTSMgr]  Unloading model " << name << std::endl;
        SherpaOnnxDestroyOfflineTts(model);
    }
    tts_models_.clear();
}

bool TTSMgr::generate(std::string text, std::string wavname){
    int sid = 0; // speaker id

    std::cout << "[TTSMgr]  Generating audio" << std::endl;
    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(tts_models_["en_US-glados-high"], text.c_str(), sid, 1.0);

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
    SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, (wavname+".wav").c_str());

    // You need to free the pointers to avoid memory leak in your app
    std::cout << "[TTSMgr]  Freeing memory" << std::endl;
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

    std::cout << "[TTSMgr]  Audio generated: " << wavname << ".wav" << std::endl; 
    
    return true;
};

short TTSMgr::getInitPercent() const {
    return init_percent_;
}

std::vector<std::string> TTSMgr::getAvailableVoiceModels() const {
    std::vector<std::string> models;
    for(const auto& [name, model] : tts_models_)
        models.push_back(name);
    return models;
}