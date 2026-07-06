#include "app/AppController.hpp"
#include "system/SystemMgr.hpp"

#include "gui/GuiMgr.hpp"       // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "sound/TTSMgr.hpp"     // Clase para gestionar TTS
#include "devices/TotalMix.hpp" // Clase para gestionar driver TotalmixFX
#include "devices/Symetrix.hpp" // Clase para gestionar driver Symetrix Composer

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

    SYS_INFO("IAppControl", std::string(online_mode_ ? "ON_LINE" : "OFF_LINE") + " mode set.");

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
    return snd_->startRec_snd(captureName); 
}

bool AppController::StopRecording(std::string const& captureName) noexcept{
    return snd_->stopRec_snd(captureName); 
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
        return tts_->generate(modelName, text, wavname);
    };

TTSData AppController::getTTSData() noexcept {
    TTSData data;

    data.available_models     = tts_->getAvailableModels();
    data.loaded_models        = tts_->getLoadedModels();
    data.num_available_models = tts_->numAvailableModels();
    data.num_loaded_models    = tts_->numLoadedModels();
    data.init_percent         = static_cast<short>(100.0 * data.num_loaded_models / data.num_available_models);
    
    return data;
}

std::string AppController::getTTSProcessingText(std::string modelName) const noexcept {
    return tts_->getProccesingText(modelName);
}
