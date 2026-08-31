#pragma once

#include "logic/PacketCoreBase.hpp"

// Forward declaration
struct CommsPacket;   ///< ver logic/comms/CommsPacket.hpp
class PositionsMgr;


/**
 * @class CommsCore
 * @brief Recibe paquetes de comunicaciones ya normalizados (desde net, dds o
 *  CLI.NET) y actualiza el estado de las posiciones afectadas.
 */
class CommsCore : public PacketCoreBase {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    CommsCore();

    /**
     * @brief Destructor
     */
    ~CommsCore() override;


// Inicialización -----------------------------------------------------------------------

    /**
    * @brief Carga y valida la configuración de la lógica de comms desde un objeto JSON.
    * @details init()/close()/isInitialized() los resuelve IModule; aquí solo hace
    *  falta implementar loadConfig(), que es puro virtual.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config) override;

    /**
     * @brief Asocia el gestor de posiciones que este Core debe actualizar.
     * @param positions Gestor de posiciones (propiedad de AppController).
     */
    void setPositionsManager(PositionsMgr* positions);


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Procesa un paquete de comunicaciones ya normalizado.
     * @details Puede traer el estado de varias posiciones a la vez (una foto
     *  completa, no un delta): por cada una, actualiza su Position asociada.
     * @param packet Paquete de comunicaciones normalizado (ver CommsPacket).
     * @return @c true si se ha procesado, @c false si se ha descartado (duplicado
     *  o sin PositionsMgr asociado).
     */
    bool Dispatch(CommsPacket const& packet);


private:

/************ Variables ********************************************************/

    PositionsMgr*   positions_;     ///< Gestor de posiciones (no propietario)

};
