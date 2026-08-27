#pragma once
#include <string>
#include <vector>

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
