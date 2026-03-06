#include "sound/SoundMgr.hpp"
#include <iostream>

// General ------------------------------------------------------------------------------

SoundMgr::SoundMgr() : engine_initialized_(false) {

}

SoundMgr::~SoundMgr() {
    stop();
}

bool SoundMgr::init() {

    // No hacer nada si ya se ha iniciado
    if (engine_initialized_) return true;
    std::cout << "[SoundMgr]    Initializating sound context..." << std::endl;

    ma_result res = ma_context_init(NULL, 0, NULL, &snd_context_);
    engine_initialized_ = (res == MA_SUCCESS) ? true : false;

    return engine_initialized_;
}

bool SoundMgr::stop() {

    // No hacer nada si ya se ha cerrado.
    if (!engine_initialized_) return true;
    std::cout << "[SoundMgr]    Closing sound engine and modules..." << std::endl;

    // Limpieza (destruir) los módulos creados
    inputs_.clear();
    //#TODO
    //playbacks_.clear();

    // Desinicializar el contexto global
    ma_context_uninit(&snd_context_);
    engine_initialized_ = false;

    std::cout << "[SoundMgr]    Sound system stopped successfully." << std::endl;
    return true;
}

bool SoundMgr::listInputDevices() {
    // Si no está inicializado no se puede hacer nada
    if (!engine_initialized_) return false;

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

bool SoundMgr::listOutputDevices() {
    // Si no está inicializado no se puede hacer nada
    if (!engine_initialized_) return false;

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