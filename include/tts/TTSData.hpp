#pragma once
#include <string>

struct TTSData {
    unsigned long long ID;          ///< Identificador único de la petición
    std::string text;               ///< Texto a sintetizar
    std::string VoiceModel;         ///< Modelo de voz a utilizar
    std::string playbackDeviceName; ///< Nombre del dispositivo de salida gestionado por SoundMgr
};
