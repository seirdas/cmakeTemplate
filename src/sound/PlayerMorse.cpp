#include "sound/PlayerMorse.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "miniaudio.h"
    #include "system/SystemMgr.hpp"
    #include "datatypes/MorseDict.hpp"
    #include "sound/APM_Imp.hpp"
    #include <memory>
    #include <cmath>


    PlayerMorse::PlayerMorse(std::string const& moduleName, void* ctx, const void* device_info) :
        AudioPlaybackModule(moduleName, ctx, device_info),
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

    bool PlayerMorse::playMorse(
        std::string const& texto,
        std::string const& audioName,
        unsigned short     volume,
        bool               loop) 
    {
        if (!initialized_)
            return false;

        std::string usedName = audioName.empty() ? texto : audioName;


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
        inst->name     = usedName; 

        // Guardar en el mapa y reproducir
        {
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[usedName] = std::move(inst);

            SYS_INFO("PlaybackModule","'" + usedName + "': init playing morse...");
            ma_sound_start(&pimpl_->playing_sounds[usedName]->sound);
        }

        return true;
    }


// Parámetros del módulo ----------------------------------------------------------------

    void PlayerMorse::setToneFrequency(float hz) {
        frequency_Hz_ = hz;
    }

    void PlayerMorse::setPuntoMs(unsigned int time_ms) {
        punto_ms_ = time_ms;
    }
    
    void PlayerMorse::setRayaMs(unsigned int time_ms) {
        raya_ms_ = time_ms;
    }
    
    void PlayerMorse::setEspacioEntreSimbolos(unsigned int time_ms) {
        espacioEntreSimbolos_ = time_ms;
    }
    
    void PlayerMorse::setEspacioEntreLetras(unsigned int time_ms) {
        espacioEntreLetras_ = time_ms;
    }
    
    void PlayerMorse::setEspacioEntreMorse(unsigned int time_ms) {
        espacioEntreMorse_ = time_ms;
    }
    
    void PlayerMorse::setSampleRate(unsigned int hz) {
        sampleRate_ = hz;
    }


    // Generación ---------------------------------------------------------------------------

    std::vector<float> PlayerMorse::generate_morse_audio(std::string const& texto) {

        std::vector<float> audio;

        // Recorrer el texto letra por letra
        for (size_t c = 0; c < texto.size(); ++c) {

            unsigned char letra_mayuscula = std::toupper(static_cast<unsigned char>(texto[c]));

            // Espacio: separación entre palabras
            if (letra_mayuscula == ' ') {
                size_t silentSamples = sampleRate_ * espacioEntreMorse_ / 1000;
                for (size_t i = 0; i < silentSamples; ++i)
                    audio.push_back(0.0f);
                continue;
            }

            // Letra no soportada por el diccionario: se ignora
            if (MORSE_DICT.find(letra_mayuscula) == MORSE_DICT.end())
                continue;

            std::string code = MORSE_DICT.at(letra_mayuscula);

            // Generar el pitido de cada letra: puntos, rayas y espacio entre símbolos
            for (size_t s = 0; s < code.size(); ++s) {

                // Punto o raya, según el símbolo
                unsigned int toneMs = (code[s] == '-') ? raya_ms_ : punto_ms_;
                size_t toneSamples = sampleRate_ * toneMs / 1000;

                // Generar el tono (onda senoidal) y guardarlo en audio
                for (size_t i = 0; i < toneSamples; ++i) {
                    float t = static_cast<float>(i) / static_cast<float>(sampleRate_);
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


#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
PlayerMorse::PlayerMorse(std::string const& moduleName, void*, const void*) :
    AudioPlaybackModule(moduleName, nullptr, nullptr)
{}


// Ejecución ----------------------------------------------------------------------------
bool PlayerMorse::playMorse(std::string const&,std::string const&,unsigned short,bool) { return false; }

// Parámetros del módulo ----------------------------------------------------------------
void PlayerMorse::setToneFrequency(float)                 { return; }
void PlayerMorse::setPuntoMs(unsigned int)                { return; }
void PlayerMorse::setRayaMs(unsigned int)                 { return; }
void PlayerMorse::setEspacioEntreSimbolos(unsigned int)   { return; }
void PlayerMorse::setEspacioEntreLetras(unsigned int)     { return; }
void PlayerMorse::setEspacioEntreMorse(unsigned int)      { return; }
void PlayerMorse::setSampleRate(unsigned int)             { return; }

// Generación ---------------------------------------------------------------------------
std::vector<float> PlayerMorse::generate_morse_audio(std::string const&)   { return {}; }

#endif