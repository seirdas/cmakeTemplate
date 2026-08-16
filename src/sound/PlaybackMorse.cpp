#include "sound/PlaybackMorse.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "miniaudio.h"
    #include "system/SystemMgr.hpp"
    #include "datatypes/MorseDict.hpp"
    #include "sound/APMImp.hpp"
    #include <memory>
    #include <cmath>


    PlaybackMorse::PlaybackMorse(void* ctx, const void* device_info)
        : AudioPlaybackModule(ctx, device_info),
        frequency_Hz_(1350),
        punto_ms_(100),
        raya_ms_(300),
        espacioEntreSimbolos_(100),
        espacioEntreLetras_(300),
        espacioEntreMorse_(500),
        sampleRate_(48000)
    {

    }


    // Ejecución ----------------------------------------------------------------------------

    bool PlaybackMorse::playMorse(
        std::string const& texto,
        std::string const& audioName,
        unsigned short     volume,
        bool               loop) 
    {
        if (!initialized_)
            return false;

        // Generar el audio a partir del texto
        std::vector<float> audio = generate_morse_audio(texto);
        if(audio.empty()) {
            SYS_WARN("AudioPlaybackModule", "playMorse: audio vacío para '" + texto + "'"); 
            return false; 
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Describir el audio generado (formato, canales, tamaño, puntero a los datos)
        ma_audio_buffer_config config = ma_audio_buffer_config_init(
            ma_format_f32,
            1, 
            audio.size(), 
            audio.data(), 
            nullptr
        );
        config.sampleRate = sampleRate_; 

        // Copiar los datos generados a un buffer propio de miniaudio
        if (ma_audio_buffer_init_copy(&config, &inst->buffer) !=MA_SUCCESS){
            SYS_WARN("AudioPlaybackModule", "playMorse: fallo al crear el buffer de audio");
            return false; 
        }
        inst->isBuffer = true; 

        // Envolver el buffer como un sonido reproducible por el motor
        if (ma_sound_init_from_data_source(&pimpl_->engine, &inst->buffer, 0, nullptr, &inst->sound) !=MA_SUCCESS){
            ma_audio_buffer_uninit(&inst->buffer);
            SYS_WARN("AudioPlaybackModule", "playMorse: fallo al inicializar el sonido");
            return false; 
        }

        //Establecer parámetros de la reproducción
        ma_sound_set_volume(&inst->sound, static_cast<float>(volume) / 100.0f);
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE :MA_FALSE);

        // Vincluar el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        //Guardar parámetros en la instancia de sonido
        inst->loopMode = loop;
        inst->name     = audioName; 

        // Guardar en el mapa y reproducir
        {
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[audioName] = std::move(inst);

            SYS_INFO("PlaybackModule","'" + audioName + "': init playing morse...");
            ma_sound_start(&pimpl_->playing_sounds[audioName]->sound);
        }

        return true;
    }


    // Generación ---------------------------------------------------------------------------

    std::vector<float> PlaybackMorse::generate_morse_audio(std::string const& texto) {
        
        // Variable para almacenar el audio generado
        std::vector<float> audio;

        // Recorrer el texto letra por letra
        for (size_t c=0; c < texto.size(); ++c){

            int letra = std::toupper(static_cast<unsigned char>(texto[c]));

            // Espacio: separación entre palabras
            if(letra == ' '){
                size_t silentSamples = sampleRate_ * espacioEntreMorse_ / 1000;
                for (size_t i = 0; i < silentSamples; ++i)
                    audio.push_back(0.0f);
                continue;
            }

            // Letra no soportada por el diccionario: se ignora
            if (MORSE_DICT.find(letra) == MORSE_DICT.end())
                continue;

            std::string code = MORSE_DICT.at(letra);

            // Generar el pitido de cada letra: puntos, rayas y espacio entre símbolos
            for (size_t s = 0; s < code.size(); ++s) {

                // Punto o raya, según el símbolo
                unsigned int toneMs = (code[s] == '-') ? raya_ms_ : punto_ms_;
                size_t toneSamples = sampleRate_ * toneMs / 1000;

                // Generar el tono (onda senoidal) y guardarlo en audio
                for (size_t i = 0; i < toneSamples; ++i) {
                    float t = static_cast<float>(i) / sampleRate_;
                    audio.push_back(sin(2.0f * 3.14159265f * frequency_Hz_ * t));
                }

                // Silencio entre símbolos de la misma letra
                if (s + 1 < code.size()) {
                    size_t gapSamples = sampleRate_ * espacioEntreSimbolos_ / 1000;
                    for (size_t i = 0; i < gapSamples; ++i)
                        audio.push_back(0.0f);
                }
            }

            // Espacio entre letras de la misma palabra
            if (c + 1 < texto.size() && texto[c + 1] != ' ') {
                size_t gapSamples = sampleRate_ * espacioEntreLetras_ / 1000;
                for (size_t i = 0; i < gapSamples; ++i)
                    audio.push_back(0.0f);
            }
        }

        return audio;
    }


// Parámetros del módulo ----------------------------------------------------------------

    void PlaybackMorse::setToneFrequency(float hz) {
        frequency_Hz_ = hz;
    }

    void PlaybackMorse::setPuntoMs(unsigned int time_ms) {
        punto_ms_ = time_ms;
    }
    
    void PlaybackMorse::setRayaMs(unsigned int time_ms) {
        raya_ms_ = time_ms;
    }
    
    void PlaybackMorse::setEspacioEntreSimbolos(unsigned int time_ms) {
        espacioEntreSimbolos_ = time_ms;
    }
    
    void PlaybackMorse::setEspacioEntreLetras(unsigned int time_ms) {
        espacioEntreLetras_ = time_ms;
    }
    
    void PlaybackMorse::setEspacioEntreMorse(unsigned int time_ms) {
        espacioEntreMorse_ = time_ms;
    }
    
    void PlaybackMorse::setSampleRate(unsigned int hz) {
        sampleRate_ = hz;
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
PlaybackMorse::PlaybackMorse(void* ctx, const void* device_info) :
    AudioPlaybackModule(nullptr, nullptr)
{}

// Ejecución ----------------------------------------------------------------------------
bool PlaybackMorse::playMorse(std::string const&,std::string const&,unsigned short,bool) { return false; }

// Parámetros del módulo ----------------------------------------------------------------
void PlaybackMorse::setToneFrequency(float)                 { return; }
void PlaybackMorse::setPuntoMs(unsigned int)                { return; }
void PlaybackMorse::setRayaMs(unsigned int)                 { return; }
void PlaybackMorse::setEspacioEntreSimbolos(unsigned int)   { return; }
void PlaybackMorse::setEspacioEntreLetras(unsigned int)     { return; }
void PlaybackMorse::setEspacioEntreMorse(unsigned int)      { return; }
void PlaybackMorse::setSampleRate(unsigned int)             { return; }

// Generación ---------------------------------------------------------------------------
std::vector<float> generate_morse_audio(std::string const&)   { return {}; }

#endif