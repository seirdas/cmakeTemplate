#include "sound/ACM_Imp.hpp"
#include "sound/AudioCaptureModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "system/SystemMgr.hpp"

    AudioCaptureModule::Impl::Impl(void* context, const void* devInfo) {
        ctx = static_cast<ma_context*>(context);
        device_info = *static_cast<const ma_device_info*>(devInfo);
    }

    void AudioCaptureModule::Impl::dataCallback_(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) {
        // Recupera el puntero a this, así el CallBack puede acceder a los miembros de la clase. 
        AudioCaptureModule* self = static_cast<AudioCaptureModule*>(pDevice->pUserData);

        // Recoge los frames capturados en este callback (samples en frameCount)
        const int16_t* samples = static_cast<const int16_t*>(pInput);

        // Filtrar a las muestras del canal seleccionado (si aplica, osea, si channelSelected=0)
        bool captureAll = (self->selectedChannel_ == 0);
        std::vector<int16_t> filteredSamples;
        unsigned int numSamples = (captureAll) ? frameCount * self->channels_ : frameCount;
        filteredSamples.reserve(numSamples);

        if (captureAll)
            // Opción 1: Guarda todos los samples de todos los canales
            filteredSamples.assign(samples, samples + numSamples);
        else {
            // Opción 2: Filtrar un canal específico
            // Ajustamos el índice: si selecciona 1, restamos 1 para acceder al índice 0
            int idx = self->selectedChannel_ - 1; 
            
            // Protegemos contra índices inválidos
            if (idx >= 0 && idx < static_cast<int>(self->channels_))
                for (unsigned int i = 0; i < frameCount; ++i)
                    filteredSamples.push_back(samples[i * self->channels_ + idx]);
            else {
                // Fallback por si fallo, grabar todos los canales
                SYS_WARN("AudioCaptureModule","Bad channel selection: capturing all device channels");
                filteredSamples.assign(samples, samples + numSamples);
            }
        }

        // 1. NIVEL DE SEÑAL: Procesar frame de muestras de captura (normales) y limpiar buffer cuando se llene
        self->captureBuffer_.insert(self->captureBuffer_.end(), filteredSamples.begin(), filteredSamples.end());

        // Procesamos solo cuando el buffer acumulado alcance el tamaño deseado
        ma_uint32 targetSize = self->processBufferSize_ * (captureAll ? self->channels_ : 1);
        if (self->captureBuffer_.size() >= targetSize) {

            // Variable temporal para almacenar valores
            float rawValue = 0;


            /* Valor de pico del buffer (0-100)*/
            int32_t peak = 0;       // 32 bits para evitar overflow con el valor -32768 
            int32_t sampleAbs = 0;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleAbs = std::abs(static_cast<int32_t>(self->captureBuffer_[i]));
                if (sampleAbs > peak)
                    peak = sampleAbs;
            }
            rawValue = static_cast<float>((peak / static_cast<float>(self->max_int16_val_)) * 100.0f); //de 0 a 100
            self->peakLevel_ = (self->smoothedValues_) ? self->smooth_level(rawValue, self->peakLevel_) : rawValue;
            if (self->peakLevel_ > 100.0f) self->peakLevel_ = 100.0f; // capar al máximo
            

            /* Valor RMS (0-100)*/
            double sampleVal = 0.0f;
            double sum = 0.0f;
            for (unsigned int i = 0; i < targetSize; ++i) {
                sampleVal = static_cast<double>(self->captureBuffer_[i]);
                sum += sampleVal * sampleVal;
            }
            double rms = std::sqrt(sum / targetSize); // La raiz es más eficiente hacerla fuera del bucle
            rawValue  = static_cast<float>((rms  / self->max_int16_val_) * 100.0); // de 0 a 100
            self->rmsLevel_ = (self->smoothedValues_) ? self->smooth_level(rawValue, self->rmsLevel_) : rawValue;
            if (self->rmsLevel_ > 100.0f) self->rmsLevel_ = 100.0f;

        
            /* Limpieza de buffer */
            self->captureBuffer_.clear();
        }

        // 2. GRABACIÓN: Guarda los samples en el buffer de grabación si está grabando. Cada frame tiene una muestra por canal
        if (self->recording_)
            self->rec_buffer_.insert(self->rec_buffer_.end(), filteredSamples.begin(), filteredSamples.end());

        // 3. CALLBACK: Envío de trama de datos de audio de entrada a "otro sitio" si el callback está definido
        {
            std::lock_guard<std::mutex> lk(self->onFrame_mtx_);
            if (self->onFrame_cb_ != nullptr) {
                self->onFrame_cb_(filteredSamples.data(), filteredSamples.size());
            }
        }
    }

    void AudioCaptureModule::Impl::notificationCallback_(const ma_device_notification* pNotification) {

        AudioCaptureModule* self = static_cast<AudioCaptureModule*>(pNotification->pDevice->pUserData);

        // Avisar, guardar y notificar si el dispositivo se ha desconectado o ya no está disponible
        if (pNotification->type == ma_device_notification_type_stopped) {
            self->is_valid_ = false;
            if (self->running_) // Avisa si no se está cerrando
                SYS_WARN("AudioCaptureModule", "Device disconnected or stopped unexpectedly.");
        }
        else {
            self->is_valid_ = true;
            SYS_INFO("AudioCaptureModule", "Device connected.");
        }
    }

#else



#endif