#include "sound/SoundMgr.hpp"
#include "sound/AudioInputModule.hpp"
#include "sound/AudioPlaybackModule.hpp"

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

    return ctx_initialized_;
}

bool SoundMgr::stop() {
    // No hacer nada si ya se ha cerrado.
    if (!ctx_initialized_) return true;
    std::cout << "[SoundMgr]    Closing sound engine and modules..." << std::endl;

    // Limpieza (destruir) los módulos creados
    inputs_.clear();
    //#TODO
    //playbacks_.clear();

    // Desinicializar el contexto global
    ma_context_uninit(&snd_context_);
    ctx_initialized_ = false;

    std::cout << "[SoundMgr]    Sound system stopped successfully." << std::endl;
    return true;
}

bool SoundMgr::listInputDevices() {
    // Si no está inicializado no se puede hacer nada
    if (!ctx_initialized_) return false;

    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;

    // Obtenemos la información del contexto
    if (ma_context_get_devices(&snd_context_, NULL, NULL, &pCaptureDeviceInfos, &captureDeviceCount) != MA_SUCCESS) {
        std::cerr << "Error al obtener dispositivos de entrada." << std::endl;
        return false;
    }

    std::cout << "\n--- DISPOSITIVOS DE ENTRADA (CAPTURE) ---" << std::endl;
    for (ma_uint32 i = 0; i < captureDeviceCount; ++i) {
        std::cout << "[" << i << "] " << pCaptureDeviceInfos[i].name;
        if (pCaptureDeviceInfos[i].isDefault) std::cout << " (Predeterminado)";
        std::cout << std::endl;
    }

    return true;
}

std::vector<std::string> SoundMgr::getAvailableInputDevices() {
    // Si no está inicializado no se puede hacer nada
    if (!ctx_initialized_) return {};

    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;

    // Obtenemos la información del contexto
    if (ma_context_get_devices(&snd_context_, NULL, NULL, &pCaptureDeviceInfos, &captureDeviceCount) != MA_SUCCESS) {
        std::cerr << "Error al obtener dispositivos de entrada." << std::endl;
        return {};
    }

    std::vector<std::string> devlist;
    for (ma_uint32 i = 0; i < captureDeviceCount; ++i) 
        devlist.push_back(pCaptureDeviceInfos[i].name);
    return devlist;
}


bool SoundMgr::listOutputDevices() {
    // Si no está inicializado no se puede hacer nada
    if (!ctx_initialized_) return false;

    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;

    // El tercer y cuarto parámetro son para Playback
    if (ma_context_get_devices(&snd_context_, &pPlaybackDeviceInfos, &playbackDeviceCount, NULL, NULL) != MA_SUCCESS) {
        std::cerr << "Error al obtener dispositivos de salida." << std::endl;
        return false;
    }

    std::cout << "\n--- DISPOSITIVOS DE SALIDA (PLAYBACK) ---" << std::endl;
    for (ma_uint32 i = 0; i < playbackDeviceCount; ++i) {
        std::cout << "[" << i << "] " << pPlaybackDeviceInfos[i].name;
        if (pPlaybackDeviceInfos[i].isDefault) std::cout << " (Predeterminado)";
        std::cout << std::endl;
    }

    return true;
}

bool SoundMgr::addCaptureDevice(std::string const& name, unsigned short index){
    


    return false;
}



bool SoundMgr::playbackTest() {

    if (!ctx_initialized_) return false;

    // Obtener lista de dispositivos de salida
    ma_device_info* pPlaybackDeviceInfos = nullptr;
    ma_uint32 playbackDeviceCount = 0;

    if (ma_context_get_devices(&snd_context_, &pPlaybackDeviceInfos, &playbackDeviceCount, nullptr, nullptr) != MA_SUCCESS)
    {
        std::cerr << "[SoundMgr] Failed to get playback devices." << std::endl;
        return false;
    }

    if (playbackDeviceCount == 0)
    {
        std::cerr << "[SoundMgr] No playback devices found." << std::endl;
        return false;
    }

    // Tomamos el primer dispositivo por defecto
    ma_device_info device = pPlaybackDeviceInfos[0];
    std::cout << "[SoundMgr] Using playback device: " << device.name << std::endl;

    // Crear módulo de reproducción
    AudioPlaybackModule playback(&snd_context_, device.id, device.name);

    if (!playback.start())
    {
        std::cerr << "[SoundMgr] Failed to start playback module." << std::endl;
        return false;
    }

    /* precarga opcional */
    playback.preload("audio/DefaultDance.mp3");
    playback.preload("audio/ding.mp3");

    /* reproducir */
    SoundID ding = playback.play(
        "audio/ding.mp3",
        1.0f,
        1.0f,
        LoopMode::LOOP);

    SoundID click = playback.play("audio/DefaultDance.mp3");

    std::cout << "[SoundMgr] Sleep for 500ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    
    /* modificar mientras reproduce */
    playback.setVolume(ding, 0.4f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    playback.setVolume(click, 0.3f);
    playback.setPitch(click, 1.2f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    
    /* cortar música */
    playback.stopSound(ding);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

    // Opcional: limpiar el módulo (destruir sonidos)
    playback.stop();

    return true;
}