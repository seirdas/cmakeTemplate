#include <filesystem>           // Controla directorios, rutas, etc.
#include "AppController.hpp"    // Clase controladora de aplicación

// TEST TTS
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>
#include <vector>
#include <string>
#include "sound/TTS.hpp"

struct AudioBuffer {
    std::vector<float> samples;
    size_t cursor = 0;
};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioBuffer* pBuffer = (AudioBuffer*)pDevice->pUserData;
    float* pOut = (float*)pOutput;
    if (!pBuffer) return;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        if (pBuffer->cursor < pBuffer->samples.size()) {
            pOut[i] = pBuffer->samples[pBuffer->cursor++];
        } else {
            pOut[i] = 0.0f;
        }
    }
}


int main(int argc, char** argv) {
    if (argc > 0) std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

    try {
        KokoroFullEngine tts;
        AudioBuffer buffer;
        
        std::string input_text = "Good evening, Southwest. The numbers are 124, 1234, 34216, 43265 and 435346, then 213 24 576 and 1223. You are receiving ATIS information Alpha. Wind 280 degrees at knots. Visibility kilometers. Clear skies. Temperature degrees Celsius, dew point degrees Celsius. Altimeter inches of mercury. Runway left in use. Advise on initial contact you have information Alpha.";
        std::cout << "Sintetizando con Phonemizer de HuggingFace..." << std::endl;
        buffer.samples = tts.synthesize(input_text);

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = 1;
        config.sampleRate = 24000;
        config.dataCallback = data_callback;
        config.pUserData = &buffer;

        ma_device device;
        if (ma_device_init(NULL, &config, &device) == MA_SUCCESS) {
            ma_device_start(&device);
            std::cout << "Reproduciendo... Presiona Enter para salir." << std::endl;
            std::cin.get();
            ma_device_uninit(&device);
        }
    } catch (const std::exception& e) {
        std::cerr << "Fallo: " << e.what() << std::endl;
    }



    // Instancia controladora de la aplicación
    AppController App;
    if (App.init())
        return App.run();
    /*else*/ return -1;
}
