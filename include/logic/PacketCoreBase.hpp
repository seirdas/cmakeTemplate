#pragma once

#include "app/IModule.hpp"
#include <mutex>
#include <cstddef>


/**
 * @class PacketCoreBase
 * @brief Base común para los "Core" de dominio (Comms, Tones, TTS) que reciben
 *  paquetes ya normalizados desde net, dds o CLI.NET.
 * @details Añade sobre IModule (init/close/isInitialized/setController ya
 *  resueltos ahí) la deduplicación de paquetes repetidos: net/dds/CLI.NET llaman
 *  a Dispatch() desde hilos distintos y pueden reenviar el mismo paquete más de
 *  una vez.
 */
class PacketCoreBase : public IModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    PacketCoreBase();

    /**
     * @brief Destructor
     */
    ~PacketCoreBase() override;


protected:

    /**
     * @brief Comprueba si el paquete recibido es un duplicado exacto del último procesado.
     * @details Protegido con mutex porque net/dds/CLI.NET llaman a Dispatch() desde hilos distintos.
     * @param data Datos representativos del paquete (p.ej. sus campos concatenados)
     * @param size Tamaño en bytes de @p data
     * @return @c true si es un duplicado del último paquete recibido, @c false en caso contrario
     */
    bool isDuplicatePacket(void const* data, std::size_t size);


private:

/************ Variables ********************************************************/

    std::mutex      dedup_mtx_;         ///< Protege last_packet_hash_ entre hilos
    unsigned long   last_packet_hash_;  ///< Hash del último paquete procesado, para descartar duplicados

};
