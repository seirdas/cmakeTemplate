#pragma once

#include <iostream>
#include <thread>               // num threads (para generación audios de voces)
#include <unordered_map>
#include "sherpa-onnx/c-api/c-api.h"

/**
 * @brief Clase TTS para generar audios usando la librería Sherpa TTS
 * @note Voices downloaded from https://k2-fsa.github.io/sherpa/onnx/tts/all/index.html
 * @see lib-sherpaonnx.cmake
 */
class TTSMgr {

// General ------------------------------------------------------------------------------

public:

    TTSMgr(std::size_t const& num_threads_ = std::thread::hardware_concurrency()) :
    num_threads_(num_threads_ == 0 ? 1 : num_threads_)
    {};

    ~TTSMgr(){
        // Destruye los modelos creados
        for (auto& [name,model] : tts_models_)
            SherpaOnnxDestroyOfflineTts(model);
            
    };

    bool init(){
        std::cout << "[TTSMgr]  Creating Sherpa config..." << std::endl;
        SherpaOnnxOfflineTtsConfig config;
        memset(&config, 0, sizeof(config));

        std::cout << "[TTSMgr]  Assigning voice model..." << std::endl;
        config.model.vits.model         = "./voices/vits-piper-en_GB-alba-medium/en_GB-alba-medium.onnx";
        config.model.vits.tokens        = "./voices/vits-piper-en_GB-alba-medium/tokens.txt";
        config.model.vits.data_dir      = "./voices/vits-piper-en_GB-alba-medium/espeak-ng-data";
        config.model.vits.noise_scale   = 0.667f; // Controla la expresividad/varianza
        config.model.vits.noise_scale_w = 0.8f;   // Varianza en la duración de los fonemas
        config.model.vits.length_scale  = 1.0f;   // 1.0 = normal, >1.0 más lento, <1.0 más rápido
        config.model.num_threads = (num_threads_>MAX_THREADS_) ? MAX_THREADS_ : num_threads_;
        config.model.debug = 1;         // 1 para logs en consola
        std::cout << "[TTSMgr]  Initializating offline tts" << std::endl;
        
        const SherpaOnnxOfflineTts* model = SherpaOnnxCreateOfflineTts(&config);

        if (!model) {
            std::string err = "[TTSMgr]  ERROR Cannot load voice model";
            std::cerr << err << std::endl;
            #ifdef _WIN32
                MessageBoxA(NULL, err.c_str(), "ERROR", MB_ICONERROR | MB_OK);
            #else
                std::string comando = "zenity --error --title=\"TTS ERROR\" --text=\"" + msg + "\" 2>/dev/null";
                system(comando.c_str());
            #endif
            return false;
        }
        
        tts_models_["en-GB-alba-medium"] = model;
        return true;
    }

    void generate(std::string text, std::string wavname){
        int sid = 0; // speaker id

        std::cout << "[TTSMgr]  Generating audio" << std::endl;
        const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(tts_models_["en-GB-alba-medium"], text.c_str(), sid, 1.0);

        std::cout << "[TTSMgr]  Writing to file..." << std::endl;
        SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, "./test.wav");

        // You need to free the pointers to avoid memory leak in your app
        std::cout << "[TTSMgr]  Freeing memory" << std::endl;
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        
        printf("Saved to ./test.wav\n");
    };

private:
    using TTSModelsMap = std::unordered_map<std::string, const SherpaOnnxOfflineTts*>;

    TTSModelsMap tts_models_;                 // TTS configurado con una voz

    int32_t                     num_threads_;       // Número de hilos con los que se generarán los audios
    const int32_t               MAX_THREADS_ = 8;   // Número máximo de hilos de cada modelo            
};
