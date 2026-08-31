#pragma once

#include "logic/PacketCoreBase.hpp"

// Forward declaration
struct TonePacket;   ///< ver logic/tones/TonePacket.hpp
class PositionsMgr;


/**
 * @class TonesCore
 * @brief Recibe paquetes de tonos ya normalizados (desde net, dds o CLI.NET)
 *  y gestiona su reproducción por posición.
 */
class TonesCore : public PacketCoreBase {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    TonesCore();

    /**
     * @brief Destructor
     */
    ~TonesCore() override;


// Inicialización -----------------------------------------------------------------------

    /**
    * @brief Carga y valida la configuración de la lógica de tonos desde un objeto JSON.
    * @details init()/close()/isInitialized() los resuelve IModule; aquí solo hace
    *  falta implementar loadConfig(), que es puro virtual.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config) override;

    /**
     * @brief Asocia el gestor de posiciones (para validar a quién va dirigido cada tono).
     * @param positions Gestor de posiciones (propiedad de AppController).
     */
    void setPositionsManager(PositionsMgr* positions);


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Procesa un paquete de tonos ya normalizado.
     * @details Trae el estado de todo el catálogo de tonos a la vez (una foto
     *  completa, no un delta).
     * @param packet Paquete de tonos normalizado (ver TonePacket).
     * @return @c true si se ha procesado, @c false si se ha descartado (duplicado
     *  o sin PositionsMgr asociado).
     */
    bool Dispatch(TonePacket const& packet);


private:

/************ Variables ********************************************************/

    PositionsMgr*   positions_;     ///< Gestor de posiciones (no propietario)

};
