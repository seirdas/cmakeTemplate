#pragma once
#include <string>
#include <chrono>

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
struct TTSPacket { 
    long long           LocalID;    ///< ATIS o ATC
	unsigned int        MsgID;      ///< Número de mensaje enviado/recibido
	unsigned long long  ID_TX;      ///< ID de transmisión (SenderRemoteRadioID)
	int                 entityID;   ///< Identificación de entidad
	std::string         entityName; ///< Nombre de entidad
    std::string         texto;      ///< Texto a reproducir
    std::string         lang;       ///< Idioma del texto

};
