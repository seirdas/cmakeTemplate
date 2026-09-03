#pragma once

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "sound/AudioPlaybackModule.hpp"
    #include <queue>
    #include <unordered_map>


    // Implementación de miembros y métodos de PIMPL
    struct AudioPlaybackModule::Impl {
        
        // Aliases y Foward declarations
        struct DeviceInstance;
        struct SoundInstance;
        using ListaDispositivos = std::vector<std::unique_ptr<DeviceInstance>>;
        using PlayingSoundsList = std::unordered_map<std::string, std::unique_ptr<SoundInstance>>;
        using ColaCleanup       = std::queue<std::unique_ptr<SoundInstance>>;

        // Estructuras
        /** @brief Elemento de audio en reproducción */
        struct SoundInstance {
            std::string             name;                   ///< Nombre del audio
            ma_sound                sound;                  ///< Parámetros de estado del sonido
            ma_audio_buffer         buffer;                 ///< Buffer de sonido en memoria
            bool                    hasBuffer   = false;    ///< Indica si "buffer" está inicializado y hay que liberarlo
            bool                    loopMode    = false;    ///< Indica reproducción en bucle del sonido
            bool                    forceStop   = false;    ///< Indica si el sonido debe pararse sin acabarlo completamente
            unsigned short          volume      = 0;        ///< Volumen de reproducción (0-100)
            float                   pitch       = 1.0f;     ///< Pitch del audio
        };

        /** @brief Información de dispositivo de reproducción */
        struct DeviceInstance {

            // Información del dispositivo
            std::string             alias;                      ///< Nombre identificativo del playback
            ma_device_info          info;                       ///< Información del dispositivo
            ma_device               device;                     ///< Dispositivo (ma)
            ma_engine               engine;                     ///< Motor de dispositivo (ma)
            unsigned int            selectedChannel = 0;        ///< Canal seleccionado en este dispositivo
            unsigned int            channels        = 0;        ///< Número de canales de este dispositivo
            bool                    initialized     = false;    ///< Bandera de inicialización
            AudioPlaybackModule*    owner           = nullptr;  ///< Dueño, necesario para callbacks

            // Sonidos en reproducción
            PlayingSoundsList       playing_sounds;             ///< Sonidos en reproducción en este dispositivo
            std::mutex              playing_sounds_mtx;         ///< Mutex para lista de sonidos en reproducción


            /************ Métodos ******************/
            
            DeviceInstance() = default;
            
            ~DeviceInstance() {
                if (initialized) {
                    ma_engine_uninit(&engine);
                    ma_device_uninit(&device);
                }
            }

            // Sin copia ni movimiento
            DeviceInstance(const DeviceInstance&) = delete;
            DeviceInstance& operator=(const DeviceInstance&) = delete;
            DeviceInstance(DeviceInstance&&) = delete;
            DeviceInstance& operator=(DeviceInstance&&) = delete;
        };

        // Componentes 
        ma_context*         ctx = nullptr;              ///< Contexto de miniaudio
        ListaDispositivos   devices;                    ///< Dispositivos de reproducción 
        std::mutex          devices_mtx;                ///< Mutex para los dispositivos
        ColaCleanup         cleanup_queue;              ///< Cola de limpieza de sonidos a desinicializar


        // ------------------------

        /**
        * @brief Constructor de Impl
        *  El constructor del Impl hace el cast de los punteros opacos
        * @param context (ma_context*) Contexto de audio de miniaudio
        * @param devInfo (ma_device_info*) Información del dispositivo
        */
        Impl(void* context) {
            ctx = static_cast<ma_context*>(context);
        }

        /**
         * @brief Destructor para liberar recursos en orden inverso de creación
         */
        ~Impl() {
            // Desinicializar los dispositivos inicializados
            for (auto& dev : devices) {
                if (dev->initialized) {
                    ma_engine_uninit(&dev->engine);
                    ma_device_uninit(&dev->device);
                }
            }
        }

        /**
        * @brief Marca como finalizado el sonido que acaba de reproducirse (finaliza)
        * @details Invocado directamente desde el hilo de procesamiento en tiempo real de Miniaudio (evitar logica pesada)
        * @param userData Datos del usuario, típicamente el puntero a esta instancia.
        * @param sound Puntero al sonido que terminó de reproducirse.
        */
        static void endCallback(void* userData, ma_sound* sound) {
            auto* devInst = static_cast<DeviceInstance*>(userData);
            if (devInst && devInst->owner)
                devInst->owner->stop_and_send_to_cleanup(sound, devInst);
        }
        
        /**
         * @brief Helper para encontrar un DeviceInstance a partir del alias
         * @param deviceAlias alias de la instancia de dispositivo
         * @return DeviceInstance* ptr a instancia de datos de dispositivo
         */
        DeviceInstance* find_device(std::string const& deviceAlias) {
            
            // Proteger lista de dispositivos
            std::lock_guard<std::mutex> lock(devices_mtx);

            // Recorrer y obtener el dispositivo por alias (si inicializado)
            for (auto& dev : devices) {
                if (dev->initialized && dev->alias == deviceAlias)
                    return dev.get();
            }
            
            /*else*/ return nullptr;
        }

        // Sin copia ni movimiento
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
    };

#endif
