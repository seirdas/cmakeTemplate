#pragma once
#include <string>
#include <vector>

/**
 * @brief Volumen de escucha de un tono concreto para una posición concreta.
 * @note Proyección de un campo de `pos_vol` del ICD real (p.ej. `piloto_vol`).
 */
struct ToneVolumeEntry {
    std::string     entityName;    ///< Alias de la posición (ver Position)
    unsigned short  volume = 0;    ///< Volumen de escucha para esa posición (0 = no escucha)
};

/**
 * @brief Estado normalizado de un tono con nombre.
 * @note Proyección de `st_tone` del ICD real. `tone_id` es el nombre del campo
 *  correspondiente en el ICD (p.ej. "c_Bell", "c_GearDown", "c_n1oclock"...),
 *  para no tener que replicar cada uno de los ~100 tonos como campo propio.
 */
struct ToneState {
    std::string                     tone_id;               ///< Nombre del tono (clave del catálogo)
    bool                             enable = false;
    unsigned short                   vol = 0;               ///< Volumen de generación del tono
    bool                             loop = false;
    float                            pitch = 1.0f;
    bool                             stopOnLoop = false;
    bool                             exclusive = false;
    std::vector<ToneVolumeEntry>    volumenes;             ///< Volumen de escucha por posición
};

/**
 * @brief Paquete de tonos ya normalizado, independiente del transporte de origen.
 * @details Un único paquete trae el estado de todos los tonos del catálogo a la
 *  vez: el ICD real manda una foto completa a 100Hz.
 */
struct TonePacket {
    std::vector<ToneState> tones;
};
