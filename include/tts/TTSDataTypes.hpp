#pragma once
#include <string>
#include <chrono>

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
 * @brief Datos agrupados del módulo TTS Core (para GUI)
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
    unsigned long long      ID_Radio;               ///< Identificador de la radio ("siempre" diferente, identificaría la voz)
    unsigned long long      ID_TX;                  ///< (DINÁMICO) Identificador de transmisión (TXID = RemoteID)
    std::string             text_playing;           ///< (DINÁMICO) Texto en reproducción
};


struct TTS_myInfo : TTSMgrInfo {
    std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida de la asignación voz <-> entidad
    std::string             model_name_assigned;    ///< Nombre del modelo asociado a la entidad
};


/**
 * @brief Estructura con los datos que procesa TTSMgr (->TTSPlay)
 */
struct TTSPacket { 
    long long           LocalID;    ///< ATIS o ATC
	unsigned int        MsgID;      ///< Número de mensaje enviado/recibido
	unsigned long long  ID_TX;      ///< ID de transmisión (SenderRemoteRadioID)
	int                 entityID;   ///< Identificación de entidad
	std::string         entityName; ///< Nombre de entidad
    std::string         texto;      ///< Texto a reproducir
    std::string         lang;       ///< Idioma del texto

};
