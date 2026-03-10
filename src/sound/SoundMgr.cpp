#include "sound/SoundMgr.hpp"
#include "sound/AudioInputModule.hpp"
#include "sound/AudioPlaybackModule.hpp"
#include <miniaudio.h>

#include <memory>
#include <iostream>
#include <chrono>               // Controla tiempos de espera
#include <thread>

// General ------------------------------------------------------------------------------

SoundMgr::SoundMgr() : ctx_initialized_(false) {

}

SoundMgr::~SoundMgr() {
    stop();
}

bool SoundMgr::init() {

    // No hacer nada si ya se ha iniciado
    if (ctx_initialized_) return true;
    std::cout << "[SoundMgr]    Initializating sound context..." << std::endl;

    ma_result res = ma_context_init(NULL, 0, NULL, &snd_context_);
    ctx_initialized_ = (res == MA_SUCCESS) ? true : false;

    if (!updateDevices()){
        std::cerr << "[SoundMgr]    ERROR Failed to get playback devices." << std::endl;
        // Mantiene el SoundMgr vivo
    }

    return ctx_initialized_;
}

bool SoundMgr::stop() {
    // No hacer nada si ya se ha cerrado.
    if (!ctx_initialized_) return true;
    std::cout << "[SoundMgr]    Closing sound engine and modules..." << std::endl;

    // Limpieza (destruir) los módulos creados
    inputs_.clear();
    playbacks_.clear();

    // Desinicializar el contexto global
    ma_context_uninit(&snd_context_);
    ctx_initialized_ = false;

    std::cout << "[SoundMgr]    Sound system stopped successfully." << std::endl;
    return true;
}

bool SoundMgr::updateDevices() {
    ma_result res = ma_context_get_devices(&snd_context_,
        &pPlaybackDevInfos_, &PlaybackDevCount_, 
        &pCaptureDeviceInfos_, &captureDeviceCount_);
    return (res==MA_SUCCESS) ? true : false;
}


// Capture Input ------------------------------------------------------------------------

std::vector<std::string> SoundMgr::getAvailableInputs() const {
    // Si no está inicializado no se puede hacer nada
    if (!ctx_initialized_) return {};

    std::vector<std::string> devlist;
    for (ma_uint32 i = 0; i < captureDeviceCount_; ++i) 
        devlist.push_back(pCaptureDeviceInfos_[i].name);
    return devlist;
}

void SoundMgr::listAvailableInputs() const {
    std::cout << "\n--- DISPOSITIVOS DE ENTRADA (CAPTURE) ---" << std::endl;
    for (ma_uint32 i = 0; i < captureDeviceCount_; ++i) {
        std::cout << "[" << i << "] " << pCaptureDeviceInfos_[i].name;
        if (pCaptureDeviceInfos_[i].isDefault) std::cout << " (Predeterminado)";
        std::cout << std::endl;
    }
}

bool SoundMgr::addCaptureDevice(std::string const& name, unsigned short index){
    // #TODO
    return false;
}


// Playbacks ----------------------------------------------------------------------------

std::vector<std::string> SoundMgr::getAvailablePlaybacks() const {
    // Si no está inicializado no se puede hacer nada
    if (!ctx_initialized_) return {};

    std::vector<std::string> devlist;
    for (ma_uint32 i = 0; i < PlaybackDevCount_; ++i) 
        devlist.push_back(pPlaybackDevInfos_[i].name);
    return devlist;
}

void SoundMgr::listAvailablePlaybacks() const {
    std::cout << "\n--- DISPOSITIVOS DE REPRODUCCIÓN (PLAYBACK) ---" << std::endl;
    for (ma_uint32 i = 0; i < PlaybackDevCount_; ++i) {
        std::cout << "[" << i << "] " << pPlaybackDevInfos_[i].name;
        if (pPlaybackDevInfos_[i].isDefault) std::cout << " (Predeterminado)";
        std::cout << std::endl;
    }
}

bool SoundMgr::addPlaybackDevice(std::string const& deviceName, std::string const& AudioFilesFolder) {
    if (!ctx_initialized_) {
        std::cerr << "[SoundMgr]    ERROR Audio context not initialized." << std::endl;
        return false;
    }

    ma_device_info* selectedDeviceInfo = nullptr;

    // Refrescar la lista de dispositivos disponibles
    updateDevices();

    // bucle para encontrar el ma_device_info por el nombre
    for (ma_uint32 i = 0; i < PlaybackDevCount_; ++i) {
        if (deviceName == pPlaybackDevInfos_[i].name) {
            selectedDeviceInfo = &pPlaybackDevInfos_[i];
            break;
        }
    }

    // Si no ha encontrado nada saltar fallo y return
    if (selectedDeviceInfo == nullptr) {
        std::cerr << "[SoundMgr]    ERROR Failed to found device with name" << deviceName << std::endl;
        return false;
    }
    std::cout << "[SoundMgr] Using playback device: " << selectedDeviceInfo->name << std::endl;

    // Crear receiver (aún no registrado) #TODO AÑADIR AudioFilesFolder
    std::unique_ptr<AudioPlaybackModule> apm = std::make_unique<AudioPlaybackModule>(&snd_context_, *selectedDeviceInfo);

    // Intentar inicializar
    std::cout << "[SoundMgr]  Initializing playback..." << std::endl;
    if (!apm->start())
    {
        // No hay nada que limpiar, el puntero make_unique se destruye al salir.
        std::cerr << "[SoundMgr]  Failed to initialize playback " << std::endl;
        return false;
    }

    // Insertar en el vector
    playbacks_.push_back(std::move(apm));
    std::cout << "[SoundMgr]  Playback loaded. "<< std::endl;

    return true;
}

bool SoundMgr::removePlaybackDevice(unsigned short index) {

    // comprobar si existe
    if (index >= playbacks_.size()) {
        std::cerr << "[SoundMgr]  Selected index " << index;
        std::cerr << " out of bounds (" << playbacks_.size() <<")" << std::endl;
        return false;
    }
    
    AudioPlaybackModule* apm = playbacks_[index].get();

    apm->stop();

    // Borrar el elemento del vector usando iterator
    playbacks_.erase(playbacks_.begin() + index);

    std::cout << "[SoundMgr] Deleted Playback." << std::endl;
    return true;
}

bool SoundMgr::playbackTest() {

    if (!ctx_initialized_) {
        std::cerr << "[SoundMgr]    ERROR Audio context not initialized." << std::endl;
        return false;
    }

    // Actualizar dispositivos disponibles
    updateDevices();

    // Tomar el PRIMER dispositivo de audio
    std::vector<std::string> list = getAvailablePlaybacks();
    if (list.empty())
        return false;
    addPlaybackDevice(list[0], "audio");        // <-- Aquí se hace también start()

    // puntero al APM que acabamos de meter
    if (playbacks_.empty()) 
        return false;
    AudioPlaybackModule* ultimoAPM = playbacks_.back().get();
    std::cout << "[SoundMgr] Testing device: " << ultimoAPM->deviceName() << std::endl;

    /* precarga opcional */
    ultimoAPM->preload("audio/DefaultDance.mp3");
    ultimoAPM->preload("audio/ding.mp3");

    /* reproducir */
    SoundID ding = ultimoAPM->play(
        "audio/ding.mp3",
        1.0f,
        1.0f,
        LoopMode::LOOP);

    SoundID click = ultimoAPM->play("audio/DefaultDance.mp3");

    std::cout << "[SoundMgr] Sleep for 500ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    
    /* modificar mientras reproduce */
    ultimoAPM->setVolume(ding, 0.4f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    ultimoAPM->setVolume(click, 0.3f);
    ultimoAPM->setPitch(click, 1.2f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    
    /* cortar música */
    ultimoAPM->stopSound(ding);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

    // Opcional: limpiar el módulo (destruir sonidos)
    ultimoAPM->stop();

    // Remover el APM de la lista
    removePlaybackDevice(static_cast<unsigned short>(playbacks_.size() - 1));

    return true;
}
