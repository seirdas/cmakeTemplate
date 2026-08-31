#pragma once
#include <string>
#include <vector>

/**
 * @brief Un TX activo dentro de una posición.
 * @note Proyección de `st_tx` del ICD real (se omite `speaker_out`, de momento
 *  no lo necesita CommsCore; se añadirá cuando se conecte el enrutado de audio).
 */
struct CommsTxSlot {
    unsigned long long id = 0;     ///< Identificador de radio/canal transmitiendo
};

/**
 * @brief Un RX activo dentro de una posición.
 * @note Proyección de `st_rx` del ICD real (se omiten `rad_type`/`rad_alias`,
 *  de momento no los necesita CommsCore; son para efectos de audio).
 */
struct CommsRxSlot {
    unsigned long long id  = 0;    ///< Identificador de radio/canal recibido
    unsigned int        vol = 0;   ///< Volumen de esa recepción
};

/**
 * @brief Estado normalizado de una posición dentro de un CommsPacket.
 * @note Proyección de `PositionSlotData` del ICD real: solo los campos que
 *  CommsCore necesita hoy.
 */
struct CommsPositionUpdate {
    std::string                entityName;       ///< Alias de la posición (ver Position), traducido desde el enum POSITIONS del ICD
    std::vector<CommsTxSlot>   tx;                ///< TX activos en este ciclo (foto completa, no delta)
    std::vector<CommsRxSlot>   rx;                ///< RX activos en este ciclo (foto completa, no delta)
    unsigned int                speaker_vol = 0;
    unsigned int                vox_level   = 0;
    bool                        mic_enabled = false;   ///< El MIC_ENABLED completo (tipo de mic) se añadirá más adelante
};

/**
 * @brief Paquete de comunicaciones ya normalizado, independiente del transporte
 *  de origen (UDP/ICD, DDS/IDL o CLI.NET/iComm).
 * @details Un único paquete trae el estado de varias posiciones a la vez: el
 *  ICD real de Host manda una foto completa de hasta 20 slots a 100Hz.
 */
struct CommsPacket {
    std::vector<CommsPositionUpdate> positions;
};
