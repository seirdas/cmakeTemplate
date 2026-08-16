
#pragma once

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "sound/AudioPlaybackModule.hpp"
    #include <queue>
    #include <unordered_map>


// Implementación de miembros y métodos de PIMPL
    struct AudioPlaybackModule::Impl {

        // Estructuras y enumerados

        /** @brief Elemento de audio en reproducción */
        struct SoundInstance {
            ma_sound        sound;              ///< La instancia del sonido en mini audio
            std::string     name;               ///< Nombre del audio
            bool            loopMode   = false; ///< Indica reproducción en bucle del sonido
            bool            forceStop  = false; ///< Indica si el sonido debe pararse sin acabarlo completamente
            ma_audio_buffer buffer;             ///< Buffer en memoria (solo si el sonido viene de un morse, no de archivo)
            bool            isBuffer = false;   ///< Indica si "buffer" está inicializado y hay que liberarlo
        };

        /** @brief Elemento de caché de archivo de audio */
        struct AudioCacheInstance {
            ma_sound    sound;                              ///< Archivo de audio cacheado
            std::chrono::steady_clock::time_point time;     ///< Marca de tiempo en el que se cacheó el audio
        };


        // Aliases
        using PlayingSoundsList = std::unordered_map<std::string, std::unique_ptr<SoundInstance>>;
        using CacheList         = std::unordered_map<std::string, std::unique_ptr<AudioCacheInstance>>;
        using ColaCleanup       = std::queue<std::unique_ptr<SoundInstance>>;

        // Componentes de miniaudio
        ma_context*     ctx;                    ///< Contexto de miniaudio
        ma_device_info  device_info;            ///< Información del dispositivo de audio
        ma_engine       engine;                 ///< Motor de audio

        // Listas de sonidos
        PlayingSoundsList   playing_sounds;     ///< Mapa de instancias de sonido reproduciéndose (nombreWav -> SoundInstance)
        CacheList           sounds_cache;       ///< Mapa de caché de archivos de audio
        ColaCleanup         cleanup_queue;      ///< Cola de limpieza de sonidos a desinicializar
        
        /**
        * @brief Constructor de Impl
        *  El constructor del Impl hace el cast de los punteros opacos
        * @param context (ma_context*) Contexto de audio de miniaudio
        * @param devInfo (ma_device_info*) Información del dispositivo
        */
        inline Impl(void* context, const void* devInfo) {
            ctx = static_cast<ma_context*>(context);
            device_info = *static_cast<const ma_device_info*>(devInfo);
        }

        /**
        * @brief Marca como finalizado el sonido que acaba de reproducirse (finaliza)
        * @details Invocado directamente desde el hilo de procesamiento en tiempo real de Miniaudio (evitar logica pesada)
        * @param userData Datos del usuario, típicamente el puntero a esta instancia.
        * @param sound Puntero al sonido que terminó de reproducirse.
        */
        inline static void endCallback(void* userData, ma_sound* sound) {
            auto* self = reinterpret_cast<AudioPlaybackModule*>(userData);
            // Busca el sonido en la lista de sonidos reproduciéndose para eliminarlo
            self->stop_and_send_to_cleanup(sound);
        }
    };

#endif