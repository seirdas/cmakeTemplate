#define MINIAUDIO_IMPLEMENTATION
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <onnxruntime_cxx_api.h>
#include <json.hpp>

using json = nlohmann::json;

class KokoroTTS {
public:
    KokoroTTS() : env(ORT_LOGGING_LEVEL_WARNING, "KokoroTTS") {
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(4);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        std::cout << "Cargando modelo..." << std::endl;
        session = std::make_unique<Ort::Session>(env, L"tts-assets/kokoro-v1.0.onnx", session_options);

        std::cout << "Cargando voces..." << std::endl;
        std::ifstream v_file("tts-assets/voices.json");
        json voices_data = json::parse(v_file);
        
        // Extraer y aplanar voz
        json raw = voices_data["af"];
        while(raw.is_array() && raw.size() > 0 && raw[0].is_array()) raw = raw[0];

        for (auto& val : raw) voice_style.push_back(val.get<float>());
        
        std::cout << "Cargando tokenizer..." << std::endl;
        std::ifstream t_file("tts-assets/phonemizer_tokenizer.json");
        tokenizer_map = json::parse(t_file)["text_symbols"];
    }

    std::vector<float> generate(const std::string& text) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // 1. Tokens
        std::vector<int64_t> tokens = { 0 }; 
        for (char c : text) {
            std::string s(1, (char)std::tolower((unsigned char)c));
            if (tokenizer_map.contains(s)) {
                tokens.push_back(tokenizer_map[s].get<int64_t>());
            }
        }
        tokens.push_back(0);

        std::vector<int64_t> tokens_shape = {1, (int64_t)tokens.size()};
        Ort::Value tokens_tensor = Ort::Value::CreateTensor<int64_t>(
            memory_info, tokens.data(), tokens.size(), tokens_shape.data(), tokens_shape.size());

        // 2. Style
        std::vector<int64_t> style_shape = {1, (int64_t)voice_style.size()}; 
        Ort::Value style_tensor = Ort::Value::CreateTensor<float>(
            memory_info, voice_style.data(), voice_style.size(), style_shape.data(), style_shape.size());

        // 3. Speed (Nuevo input requerido por tu modelo)
        std::vector<float> speed_data = { 1.0f }; // 1.0 es la velocidad normal
        std::vector<int64_t> speed_shape = { 1 };
        Ort::Value speed_tensor = Ort::Value::CreateTensor<float>(
            memory_info, speed_data.data(), speed_data.size(), speed_shape.data(), speed_shape.size());

        // Array de Inputs: Ahora son 3
        const char* input_names[] = {"tokens", "style", "speed"}; 
        const char* output_names[] = {"audio"};
        
        Ort::Value inputs[] = {std::move(tokens_tensor), std::move(style_tensor), std::move(speed_tensor)};

        // Ejecutar Inferencia (cambiado de 2 a 3 en la cantidad de inputs)
        auto outputs = session->Run(Ort::RunOptions{nullptr}, input_names, inputs, 3, output_names, 1);
        
        // Recuperar audio
        float* audio_ptr = outputs[0].GetTensorMutableData<float>();
        size_t sample_count = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        return std::vector<float>(audio_ptr, audio_ptr + sample_count);
    }

private:
    Ort::Env env;
    std::unique_ptr<Ort::Session> session;
    std::vector<float> voice_style;
    json tokenizer_map;
};
