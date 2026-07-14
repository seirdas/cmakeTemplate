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

/**
 * @brief Estructura con los datos que procesa TTSMgr (->TTSPlay)
 */
struct TTSMgrInfo{
    std::string             entity_name;            ///< Nombre de la entidad asociada
    std::string             model_name_assigned;    ///< Nombre del modelo asociado a la entidad
    std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida de la asignación voz <-> entidad
    unsigned long long      ID_Radio;               ///< Identificador de la radio ("siempre" diferente, identificaría la voz)
    unsigned long long      ID_TX;                  ///< (DINÁMICO) Identificador de transmisión (TXID = RemoteID)
    std::string             text_playing;           ///< (DINÁMICO) Texto en reproducción
};
