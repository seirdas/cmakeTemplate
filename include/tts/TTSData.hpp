#pragma once
#include <string>

/**
 * @brief Datos del paquete de envío al TTSMgr
 */
struct TTSDataPacket {
    unsigned long long ID;          ///< Identificador único de la petición
    std::string text;               ///< Texto a sintetizar
    std::string VoiceModel;         ///< Modelo de voz a utilizar
    std::string playbackDeviceName; ///< Nombre del dispositivo de salida gestionado por SoundMgr
};

/**
 * @brief Datos del módulo TTS Core
 */
struct TTSCoreData {
    short init_percent;
    short num_available_models;
    short num_loaded_models;
    std::vector<std::string> available_models;
    std::vector<std::string> loaded_models;
};