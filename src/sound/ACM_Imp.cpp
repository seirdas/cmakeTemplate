#include "sound/ACM_Imp.hpp"
#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "system/SystemMgr.hpp"
    #include <cmath>


    AudioCaptureModule::Impl::Impl(void* context) {
        ctx = static_cast<ma_context*>(context);
    }

    void AudioCaptureModule::Impl::dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) {

        // Recupera el puntero a DeviceInstance, así el CallBack puede acceder a los miembros del dispositivo. 
        DeviceInstance* devInst = static_cast<DeviceInstance*>(pDevice->pUserData);
        if (!devInst || !devInst->owner) return;

        // La clase está almacenada en la instancia, owner
        AudioCaptureModule* self = devInst->owner;

        // Recoge los frames capturados en este callback (samples en frameCount)
        const int16_t* samples = static_cast<const int16_t*>(pInput);

        // Filtrar al canal seleccionado de este dispositivo (si aplica)
        bool captureAll = (devInst->selectedChannel == 0);
        std::vector<int16_t> filteredSamples;
        unsigned int numSamples = (captureAll) ? frameCount * devInst->channels : frameCount;
        filteredSamples.reserve(numSamples);

        if (captureAll)
            // Opción 1: Guarda todos los samples de todos los canales
            filteredSamples.assign(samples, samples + numSamples);
        else {
            // Opción 2: Filtrar un canal específico
            // Ajustamos el índice: si selecciona 1, restamos 1 para acceder al índice 0
            int idx = devInst->selectedChannel - 1; 
            
            // Protegemos contra índices inválidos
            if (idx >= 0 && idx < static_cast<int>(devInst->channels))
                for (unsigned int i = 0; i < frameCount; ++i)
                    filteredSamples.push_back(samples[i * devInst->channels + idx]);
            else {
                // Fallback por si fallo, grabar todos los canales
                SYS_WARN("AudioCaptureModule", "'" + devInst->alias + "': bad channel selection: capturing all device channels");
                filteredSamples.assign(samples, samples + numSamples);
            }
        }

        // 1. NIVEL DE SEÑAL: Almacenar frames de muestras de captura, y procesar y limpiar buffer cuando se llene
        {
            // Proteger el buffer de captura
            std::lock_guard<std::mutex> lk(devInst->capture_buffer_mtx);
            devInst->captureBuffer.insert(devInst->captureBuffer.end(), filteredSamples.begin(), filteredSamples.end());

            // Procesamos solo cuando el buffer acumulado alcance el tamaño deseado
            ma_uint32 targetSize = self->processBufferSize_ * (captureAll ? devInst->channels : 1);
                if (devInst->captureBuffer.size() >= targetSize) {

                // Variable temporal para almacenar valores
                float rawValue = 0;
    
                // Valor de pico del buffer (0-100)
                int32_t peak = 0, sampleAbs = 0;
                for (unsigned int i = 0; i < targetSize; ++i) {
                    sampleAbs = std::abs(static_cast<int32_t>(devInst->captureBuffer[i]));
                    if (sampleAbs > peak) 
                        peak = sampleAbs;
                }
                rawValue = static_cast<float>(peak) / static_cast<float>(self->max_int16_val_) * 100.0f;
                devInst->peakLevel = (self->smoothedValues_) ? self->smooth_level(rawValue, devInst->peakLevel) : rawValue;
                if (devInst->peakLevel > 100.0f) devInst->peakLevel = 100.0f;
                
                // Valor RMS (0-100)
                double sampleVal = 0, sum = 0;
                for (unsigned int i = 0; i < targetSize; ++i) {
                    sampleVal = static_cast<double>(devInst->captureBuffer[i]);
                    sum += sampleVal * sampleVal;
                }
                double rms = std::sqrt(sum / targetSize);
                rawValue = static_cast<float>((rms / self->max_int16_val_) * 100.0);
                devInst->rmsLevel = (self->smoothedValues_) ? self->smooth_level(rawValue, devInst->rmsLevel) : rawValue;
                if (devInst->rmsLevel > 100.0f) 
                    devInst->rmsLevel = 100.0f;
    
                // Limpieza de buffer de captura
                devInst->captureBuffer.clear();
            }
        }

        // 2. GRABACIÓN: Guarda los samples en el buffer de grabación si está grabando. Cada frame tiene una muestra por canal
        if (devInst->recording_) {
            std::lock_guard<std::mutex> lk(devInst->rec_buffer_mtx_);
            devInst->rec_buffer_.insert(devInst->rec_buffer_.end(), filteredSamples.begin(), filteredSamples.end());
        }
    }

    void AudioCaptureModule::Impl::notificationCallback_(const ma_device_notification* pNotification) {

        // Recupera el puntero a DeviceInstance, así el CallBack puede acceder a los miembros del dispositivo. 
        DeviceInstance* devInst = static_cast<DeviceInstance*>(pNotification->pDevice->pUserData);
            if (!devInst) return;

        // Avisar, guardar y notificar si el dispositivo se ha desconectado o ya no está disponible
        if (pNotification->type == ma_device_notification_type_stopped) {
            devInst->valid = false;
            if (devInst->running)
                SYS_WARN("AudioCaptureModule", "'" + devInst->alias + "': device disconnected or stopped unexpectedly.");
        }
        else {
            devInst->valid = true;
            SYS_INFO("AudioCaptureModule", "'" + devInst->alias + "': device connected.");
        }
    }

#endif
