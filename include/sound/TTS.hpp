#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cctype>
#include <onnxruntime_cxx_api.h>
#include <json.hpp>

/*  VOICES:
    "af"
    "af_bella"
    "af_nicole"
    "af_sarah"
    "af_sky"
    "am_adam"
    "am_michael"
    "bf_emma"
    "bf_isabella"
    "bm_george"
    "bm_lewis"
*/

using json = nlohmann::json;

class KokoroFullEngine {
public:
    KokoroFullEngine() : env(ORT_LOGGING_LEVEL_WARNING, "Kokoro") {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(4);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        std::cout << "Cargando Kokoro TTS..." << std::endl;
        tts_session = std::make_unique<Ort::Session>(env, L"tts-assets/kokoro-v1.0.int8.onnx", options);

        std::cout << "Cargando voces..." << std::endl;
        std::ifstream v_file("tts-assets/voices.json");
        json v_data = json::parse(v_file);
        json raw = v_data["af_nicole"]; 
        while(raw.is_array() && raw.size() > 0 && raw[0].is_array()) raw = raw[0];
        for (auto& val : raw) voice_style.push_back(val.get<float>());
        if (voice_style.size() != 256) throw std::runtime_error("Error de dimensión en la voz.");

        std::cout << "Cargando Diccionario de fonemas..." << std::endl;
        std::ifstream d_file("tts-assets/phoneme_dict.json");
        phoneme_dict = json::parse(d_file);

        // VOCABULARIO EXACTO DE KOKORO V1.0 (177 caracteres exactos, IDs del 0 al 176)
        // El error idx=209 desaparece aquí porque no asignaremos ningún ID mayor a 176.
        const char* vocab_str = "$;:,.!?¡¿—…\"«»“” ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzɑɐɒæɓʙβɔɕçɗɖðʤəɘɚɛɜɝɞɟʄɡɠɢʛɦɧħɥʜɨɪʝɭɬɫɮʟɱɯɰŋɳɲɴøɵɸθœɶʘɹɺɾɻʀʁɽʂʃʈʧʉʊʋⱱʌɣɤʍχʎʏʑʐʒʔʡʕʢǀǁǂǃˈˌːˑʼʴʰʱʲʷˠˤ˞↓↑→↗↘'̩'ᵝ";
        
        std::string v_str(vocab_str);
        int64_t id = 0; // Kokoro ONNX empieza en 0
        for (size_t i = 0; i < v_str.length();) {
            int cplen = 1;
            if ((v_str[i] & 0xf8) == 0xf0) cplen = 4;
            else if ((v_str[i] & 0xf0) == 0xe0) cplen = 3;
            else if ((v_str[i] & 0xe0) == 0xc0) cplen = 2;
            kokoro_vocab[v_str.substr(i, cplen)] = id++;
            i += cplen;
        }
    }

    std::vector<float> synthesize(const std::string& text) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // --- 1. PROCESAMIENTO DE TEXTO A TOKENS KOKORO ---
        std::vector<int64_t> kokoro_tokens; 
        std::istringstream iss(text);
        std::string word;
        bool first = true;
        
        while (iss >> word) {
            if (!first) kokoro_tokens.push_back(kokoro_vocab[" "]); // Espacio normal ' '
            first = false;

            std::string search_word = "";
            std::string punct_start = "";
            std::string punct_end = "";

            // Separar puntuación de la palabra (ej: "Hello," -> "Hello" y ",")
            for (char c : word) {
                if (std::isalpha((unsigned char)c) || c == '\'') {
                    search_word += std::tolower((unsigned char)c);
                } else if (search_word.empty()) {
                    punct_start += c;
                } else {
                    punct_end += c;
                }
            }

            // Añadir puntuación inicial (ej: ¿ o ¡)
            for (char c : punct_start) {
                std::string s(1, c);
                if (kokoro_vocab.contains(s)) kokoro_tokens.push_back(kokoro_vocab[s]);
            }

            // Buscar en el diccionario (en_us)
            std::string ipa_string;
            if (!search_word.empty()) {
                if (phoneme_dict["en_us"].contains(search_word)) {
                    ipa_string = phoneme_dict["en_us"][search_word].get<std::string>();
                } else {
                    ipa_string = search_word; // Deletrear si no existe
                }

                // Convertir caracteres IPA UTF-8 a IDs
                for (size_t i = 0; i < ipa_string.length();) {
                    int cplen = 1;
                    if ((ipa_string[i] & 0xf8) == 0xf0) cplen = 4;
                    else if ((ipa_string[i] & 0xf0) == 0xe0) cplen = 3;
                    else if ((ipa_string[i] & 0xe0) == 0xc0) cplen = 2;
                    
                    std::string utf8_char = ipa_string.substr(i, cplen);
                    if (kokoro_vocab.contains(utf8_char)) {
                        kokoro_tokens.push_back(kokoro_vocab[utf8_char]);
                    }
                    i += cplen;
                }
            }

            // Añadir puntuación final (ej: . , !)
            for (char c : punct_end) {
                std::string s(1, c);
                if (kokoro_vocab.contains(s)) kokoro_tokens.push_back(kokoro_vocab[s]);
            }
        }

        // --- 2. KOKORO TTS ---
        std::vector<int64_t> t_shape = {1, (int64_t)kokoro_tokens.size()};
        Ort::Value t_tensor = Ort::Value::CreateTensor<int64_t>(
            memory_info, kokoro_tokens.data(), kokoro_tokens.size(), t_shape.data(), t_shape.size());

        std::vector<int64_t> s_shape = {1, 256}; 
        Ort::Value s_tensor = Ort::Value::CreateTensor<float>(
            memory_info, voice_style.data(), 256, s_shape.data(), s_shape.size());

        float speed_val = 0.7f;
        std::vector<int64_t> sp_shape = {1};
        Ort::Value sp_tensor = Ort::Value::CreateTensor<float>(memory_info, &speed_val, 1, sp_shape.data(), sp_shape.size());

        const char* tts_in[] = {"tokens", "style", "speed"};
        const char* tts_out[] = {"audio"};
        
        Ort::Value tts_inputs[] = {std::move(t_tensor), std::move(s_tensor), std::move(sp_tensor)};
        auto tts_outputs = tts_session->Run(Ort::RunOptions{nullptr}, tts_in, tts_inputs, 3, tts_out, 1);
        
        float* audio_data = tts_outputs[0].GetTensorMutableData<float>();
        size_t audio_count = tts_outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        
        return std::vector<float>(audio_data, audio_data + audio_count);
    }

private:
    Ort::Env env;
    std::unique_ptr<Ort::Session> tts_session;
    std::vector<float> voice_style;
    json phoneme_dict;
    std::map<std::string, int64_t> kokoro_vocab;
};