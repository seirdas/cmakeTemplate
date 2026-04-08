#include <filesystem>           // Controla directorios, rutas, etc.
#include "AppController.hpp"    // Clase controladora de aplicación

// TEST TTS
#include <iostream>
#include <vector>
#include <string>
#include "sherpa-onnx/c-api/c-api.h"


int main(int argc, char** argv) {
    if (argc > 0) std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

    // prueba con sherpa
    std::cout << "creating Sherpa config..." << std::endl;
    SherpaOnnxOfflineTtsConfig config;
    memset(&config, 0, sizeof(config));

    std::cout << "assigning voice model..." << std::endl;
    config.model.kitten.model = "./voices/kitten-nano-en-v0_2-fp16/model.fp16.onnx";
    config.model.kitten.voices = "./voices/kitten-nano-en-v0_2-fp16/voices.bin";
    config.model.kitten.tokens = "./voices/kitten-nano-en-v0_2-fp16/tokens.txt";
    config.model.kitten.data_dir = "./voices/kitten-nano-en-v0_2-fp16/espeak-ng-data";

    config.model.num_threads = 4;
    std::cout << "Initializating offline tts" << std::endl;
    const SherpaOnnxOfflineTts *tts = SherpaOnnxCreateOfflineTts(&config);

    int sid = 0; // speaker id
    const char *text = "Good evening, Southwest 152. You are receiving ATIS information Alpha. Wind 280 degrees at 10 knots. Visibility 10 kilometers. Clear skies. Temperature 25 degrees Celsius, dew point 15 degrees Celsius. Altimeter 29.92 inches of mercury. Runway 27 left in use. Advise on initial contact you have information Alpha.";

    std::cout << "generating audio" << std::endl;
    const SherpaOnnxGeneratedAudio *audio =
        SherpaOnnxOfflineTtsGenerate(tts, text, sid, 1.0);

    std::cout << "escribiendo en archivo..." << std::endl;
    SherpaOnnxWriteWave(audio->samples, audio->n, audio->sample_rate,
                        "./test.wav");

    // You need to free the pointers to avoid memory leak in your app
    std::cout << "liberando memoria sherpa" << std::endl;
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    SherpaOnnxDestroyOfflineTts(tts);

    printf("Saved to ./test.wav\n");

    // Instancia controladora de la aplicación
    AppController App;
    if (App.init())
        return App.run();
    /*else*/ return -1;
}
