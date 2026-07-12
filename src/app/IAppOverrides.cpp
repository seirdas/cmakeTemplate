#include "app/AppController.hpp"
#include "system/SystemMgr.hpp"

#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "tts/TTSMgr.hpp"     // Clase para gestionar TTS
#include "tts/TTSData.hpp"

/* 
* Aquí solo deberían ir acciones que se ejecuten sobre otros módulos
* Para la "solicitud" de datos, se debería hacer un patrón observador
*/


/* --- IAppControl methods --- */


// Aplicación -----------------------------------------------------------------------

std::string AppController::getVersion() const noexcept { 
    return version_; 
}

void AppController::setOnlineMode(bool nuevo_online_mode) noexcept { 

    std::unique_lock<std::mutex> lock(online_mtx_);
    if(online_mode_==nuevo_online_mode) return;
    online_mode_=nuevo_online_mode;
    lock.unlock();

    SYS_INFO("IAppControl", std::string(online_mode_ ? "ONLINE" : "OFFLINE") + " mode set.");

    if(online_mode_) {
        net_->start();
        online_cv_.notify_all();
    }
    else {
        net_->stop();
        online_cv_.notify_all();
    }
};

bool AppController::isOnlineMode() const noexcept {
    return online_mode_;
};


// Sockets --------------------------------------------------------------------------

bool AppController::addReceiver() const noexcept {
    // TODO
    return false;
}

bool AppController::removeReceiver() const noexcept {
    // TODO
    return false;
}


// Audio ----------------------------------------------------------------------------

std::vector<std::string> AppController::getAvailableInputDevices() noexcept {
    return snd_->getAvailableInputs();
}

std::vector<std::string> AppController::getManagedCaptures() noexcept {
    return snd_->getManagedCaptures();
}

std::vector<std::string> AppController::getAvailablePlaybackDevices() noexcept {
    return snd_->getAvailablePlaybacks();
}

void AppController::refreshAudioDevices() noexcept {
    snd_->updateDevices();
    return;
}

bool AppController::addInputDevice(std::string const& captureName, std::string const& deviceName) noexcept {
    return snd_->addCaptureDevice(captureName, deviceName);
}

bool AppController::removeInputDevice(std::string const& captureName) noexcept {
    return snd_->removeInputDevice(captureName);
}

bool AppController::StartRecording(std::string const& captureName) noexcept{
    return snd_->startRec(captureName); 
}

bool AppController::StopRecording(std::string const& captureName) noexcept{
    return snd_->stopRec(captureName); 
}

size_t AppController::getInputBufferSize(std::string const& captureName) noexcept {
    return snd_->getInputBufferSize(captureName); 
}

size_t AppController::getInputRecBufferSize(std::string const& captureName) noexcept {
    return snd_->getInputRecBufferSize(captureName); 
}

bool AppController::isInputDeviceValid(std::string const& captureName) noexcept {
    return snd_->isInputDeviceValid(captureName);
}

float AppController::getInputRMSLevel(std::string const& captureName) noexcept {
    return snd_->getInputRmsLevel(captureName);
}

float AppController::getInputPeakLevel(std::string const& captureName) noexcept {
    return snd_->getInputPeakLevel(captureName);
}



// TTS ----------------------------------------------------------------------------------

bool AppController::TTSgenerate(
    std::string const& modelName, 
    std::string const& text, 
    std::string const& wavname)
    noexcept {
        return tts_->generateWav(modelName, text, wavname);
    };

bool AppController::TTSPlay(
    std::string const& modelName, 
    std::string const& text, 
    std::string const& playbackName)
    noexcept {
        // #TODO
        return false;
    }

TTSCoreData AppController::getTTSData() noexcept {
    TTSCoreData data;

    data.available_models     = tts_->getAvailableModels();
    data.loaded_models        = tts_->getLoadedModels();
    data.num_available_models = tts_->numAvailableModels();
    data.num_loaded_models    = tts_->numLoadedModels();
    data.init_percent         = static_cast<short>(100.0 * data.num_loaded_models / data.num_available_models);
    
    return data;
}



// VoIP ---------------------------------------------------------------------------------

bool AppController::addVoiprec(
    std::string const&  voiprecName,
    bool                isPlaybackSource,
    std::string const&  audioSourceName,
    const std::string   socketName, 
    std::string const&  ipDest,
    unsigned short      portDest) noexcept 
    {
        // 1. Definimos la acción de red mediante una lambda capturando 'this'
        // Así VoIPRec invocará de forma indirecta a NetMgr sin conocerlo.
        auto networkSendLambda = [this, socketName, ipDest, portDest](const std::vector<char>& packet) {
            // Suponiendo que conoces IP/Puerto destino de antemano o están asociados al socketName
            this->net_->sendData(socketName, packet, ipDest, portDest);
        };

        // Añadir el VoIPRec pasándole el puente de red
        
        //voip_->add_voiprec(voiprecName, networkSendLambda);

        // Conectar el flujo de audio de SoundMgr hacia ese VoIPRec específico
        // Usamos el callback expuesto de AudioInputModule implementado
        // (Nota: Tendrás que añadir un método en SoundMgr para acceder al dispositivo o setear el callback por índice/nombre)
        
        /* Suponiendo que SoundMgr te permite hacer esto: */
        
        /*
        
            this->snd_->setInputCallback(audioSourceName, [this, voiprecName](const int16_t* samples, unsigned int frameCount) {
                this->voip_.feedAudioToRec(voiprecName, samples, frameCount);
            });
        
        */

        // (de momento no funciona)
        return false;
    }