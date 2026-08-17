#pragma once

#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>

    // Implementación de miembros y métodos de la librería externa
    struct AudioCaptureModule::Impl {
        ma_context*    ctx = nullptr;   ///< Contexto de miniaudio
        ma_device      device;          ///< Dispositivo de entrada
        ma_device_info device_info;     ///< Información del dispositivo de entrada
        ma_encoder     encoder;         ///< Codificador de audio para grabación 

        /**
         * @brief Constructor de Impl
         *  El constructor del Impl hace el cast de los punteros opacos
         * @param context (ma_context*) Contexto de audio de miniaudio
         * @param devInfo (ma_device_info*) Información del dispositivo
         */
        Impl(void* context, const void* devInfo);

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
    };

#else

struct AudioCaptureModule::Impl { }

#endif