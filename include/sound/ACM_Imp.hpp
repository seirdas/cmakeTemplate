#pragma once

#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include <atomic>

    // Implementación de miembros y métodos de la librería externa
    struct AudioCaptureModule::Impl {

        // Aliases y Foward declarations
        struct DeviceInstance;
        using ListaDispositivos = std::vector<std::unique_ptr<DeviceInstance>>;

        // Estructuras
        /** @brief Instancia de un dispositivo de captura de entrada */
        struct DeviceInstance {

            // Información del dispositivo
            std::string     alias;                      ///< Nombre del alias interno (ej: "mic_main", "aux_in")
            ma_device_info  info;                       ///< Información del dispositivo de captura
            ma_device       device;                     ///< Estructura del dispositivo de captura
            unsigned int    selectedChannel = 0;        ///< Canal de captura seleccionado
            unsigned int    sampleRate      = 48000;    ///< Frecuencia de muestreo de este dispositivo
            unsigned int    channels        = 0;        ///< Número de canales configurados
            bool            initialized = false;        ///< Bandera de inicialización del ma_device
            bool    valid   = false;                    ///< Estado reportado por notificationCallback_ (conectado/activo)
            bool    running = false;                    ///< Evita el warning de "desconexión inesperada" en una parada intencional (stop/remove)
            AudioCaptureModule*     owner   = nullptr;  ///< Dueño, necesario para resolver desde los callbacks estáticos

            // Captura
            std::vector<int16_t>    captureBuffer;          ///< Buffer de captura de audio
            std::mutex              capture_buffer_mtx;     ///< Mutex para buffer de captura
            std::atomic<float>      rmsLevel        = 0.0f; ///< Nivel RMS en un "instante" (capturebuffersize)
            std::atomic<float>      peakLevel       = 0.0f; ///< Nivel de pico en un "instante" (capturebuffersize)
            
            // Grabación
            std::vector<int16_t>    rec_buffer_;            ///< Buffer de grabación de audio (a archivo)
            std::mutex              rec_buffer_mtx_;        ///< Mutex para buffer de grabación
            ma_encoder              encoder;                ///< Codificador de grabación (por dispositivo)
            bool                    encoder_inited_ = false;///< Flag que indica si el encoder se ha inicializado
            std::atomic<bool>       recording_      = false;///< Flag para guardar samples de audio en el buffer de grabación
            std::string             rec_filename_;          ///< Nombre de archivo generado con la grabación (para INFO)


            DeviceInstance() = default;

            ~DeviceInstance() {
                if (encoder_inited_)
                    ma_encoder_uninit(&encoder);
                if (initialized) {
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
        ListaDispositivos   devices;                    ///< Dispositivos de captura 
        std::mutex          devices_mtx;                ///< Mutex para los dispositivos


        /************ Métodos ******************/

        /**
         * @brief Constructor de Impl
         *  El constructor del Impl hace el cast de los punteros opacos
         * @param context (ma_context*) Contexto de audio de miniaudio
         * @param devInfo (ma_device_info*) Información del dispositivo
         */
        Impl(void* context);

        /**
         * @brief Miniaudio llama a esta función cuando el estado del dispositivo cambia 
         * @note Está definido en el Impl porque la firma de la función debe ser así, y depende de la librería
         */
        static void notificationCallback_(const ma_device_notification* pNotification);

        /** 
         * @brief Miniaudio llama a esta función automáticamente cada vez que el micrófono captura un sample de audio
         * @note Está definido en el Impl porque la firma de la función debe ser así, y depende de la librería
         */
        static void dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);

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

    };

#else

struct AudioCaptureModule::Impl { };

#endif
