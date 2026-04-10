#include "sound/TTSMgr.hpp"

#include <unordered_map>
#ifdef _WIN32
    #include <windows.h>
#endif

TTSMgr::TTSMgr(std::size_t const& num_threads_) :
    num_threads_(num_threads_ == 0 ? 1 : num_threads_),
    init_percent_(0)
{};

TTSMgr::~TTSMgr(){
    cerrar();
};

bool TTSMgr::init(){

    // Función de carga de modelo de voz
    auto load_vits_model = [this](std::string voicename){
        SherpaOnnxOfflineTtsConfig config;
        memset(&config, 0, sizeof(config));

        // Genera la configuración a partir de los nombres habituales de los archivos
        std::string onnx    = (models_path_+"/vits-piper-"+voicename+"/"+voicename+".onnx");
        std::string tokens  = (models_path_+"/vits-piper-"+voicename+"/tokens.txt");
        std::string espeak  = (models_path_+"/vits-piper-"+voicename+"/espeak-ng-data");
        config.model.vits.model         = onnx.c_str();
        config.model.vits.tokens        = tokens.c_str();
        config.model.vits.data_dir      = espeak.c_str();
        config.model.vits.noise_scale   = 0.667f; // Controla la expresividad/varianza
        config.model.vits.noise_scale_w = 0.8f;   // Varianza en la duración de los fonemas
        config.model.vits.length_scale  = 1.0f;   // 1.0 = normal, >1.0 más lento, <1.0 más rápido
        config.model.num_threads = num_threads_;
        config.model.debug = 0;         // 1 para logs en consola
        std::cout << "[TTSMgr]  Initializating voice model " << voicename << " ("<< init_percent_ <<"%)" << std::endl;
        
        const SherpaOnnxOfflineTts* tts_model = SherpaOnnxCreateOfflineTts(&config);

        if (!tts_model) {
            std::string err = "[TTSMgr]  ERROR Cannot load voice model: " + onnx;
            std::cerr << err << std::endl;
            #ifdef _WIN32
                MessageBoxA(NULL, err.c_str(), "ERROR", MB_ICONERROR | MB_OK);
            #else
                std::string comando = "zenity --error --title=\"TTS ERROR\" --text=\"" + msg + "\" 2>/dev/null";
                system(comando.c_str());
            #endif
            return false;
        }
        
        tts_models_[voicename] = tts_model;
        return true;
    };

    // El init percent debería estar en función del número de voces, de momento se pone a mano

    if(!load_vits_model("en_GB-alan-low")) 
        return false;
    init_percent_=(int) (1.0/7.0*100);
    
    if(!load_vits_model("en_GB-southern_english_female-low")) 
        return false;
    init_percent_=(int) (2.0/7.0*100);
    
    if(!load_vits_model("en_US-amy-low")) 
        return false;
    init_percent_=(int) (3.0/7.0*100);
    
    if(!load_vits_model("en_US-danny-low")) 
        return false;
    init_percent_=(int) (4.0/7.0*100);

    if(!load_vits_model("en_US-kathleen-low")) 
        return false;
    init_percent_=(int) (5.0/7.0*100);

    if(!load_vits_model("en_US-lessac-low")) 
        return false;
    init_percent_=(int) (6.0/7.0*100);

    if(!load_vits_model("en_US-ryan-low")) 
        return false;
    init_percent_=(int) (7.0/7.0*100);


    std::cout << "[TTSMgr]  TTS models loaded." << std::endl;
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
    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(tts_models_["en_US-kathleen-low"], text.c_str(), sid, 1.0);

    if (!audio) {
        std::string err = "[TTSMgr]  ERROR Cannot generate audio: " + wavname;
        std::cerr << err << std::endl;
        #ifdef _WIN32
            MessageBoxA(NULL, err.c_str(), "ERROR", MB_ICONERROR | MB_OK);
        #else
            std::string comando = "zenity --error --title=\"TTS ERROR\" --text=\"" + msg + "\" 2>/dev/null";
            system(comando.c_str());
        #endif
        return false;
    }

    std::cout << "[TTSMgr]  Writing to file..." << std::endl;
    SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate, (wavname+".wav").c_str());

    // You need to free the pointers to avoid memory leak in your app
    std::cout << "[TTSMgr]  Freeing memory" << std::endl;
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

    std::cout << "[TTSMgr]  Audio generated to" << wavname << ".wav" << std::endl; 
    
    return true;
};

short TTSMgr::getInitPercent() const {
    return init_percent_;
}