#include "logic/comms/CommsCore.hpp"
#include "logic/comms/CommsPacket.hpp"
#include "positions/PositionsMgr.hpp"
#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// General ------------------------------------------------------------------------------

CommsCore::CommsCore() :
    positions_(nullptr)
{

}

CommsCore::~CommsCore() {
    close();
}


// Inicialización -----------------------------------------------------------------------

void CommsCore::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO: parámetros propios de la lógica de comms
}

void CommsCore::setPositionsManager(PositionsMgr* positions) {
    positions_ = positions;
}


// Ejecución ----------------------------------------------------------------------------

bool CommsCore::Dispatch(CommsPacket const& packet) {

    // Construir una clave representativa del paquete completo para el dedup
    // (el ICD real manda una foto completa a 100Hz; casi siempre es igual a la anterior)
    std::string key;
    for (CommsPositionUpdate const& pos : packet.positions) {
        key += pos.entityName + '|';
        key += std::to_string(pos.speaker_vol) + '|';
        key += std::to_string(pos.vox_level) + '|';
        key += (pos.mic_enabled ? '1' : '0');
        for (CommsTxSlot const& tx : pos.tx)
            key += "t" + std::to_string(tx.id);
        for (CommsRxSlot const& rx : pos.rx)
            key += "r" + std::to_string(rx.id) + ":" + std::to_string(rx.vol);
        key += ';';
    }

    if (isDuplicatePacket(key.data(), key.size()))
        return false;

    if (!positions_) {
        SYS_WARN("CommsCore","Dispatch called without a PositionsMgr assigned");
        return false;
    }

    // Negocio: aplicar el estado de cada posición del paquete a su Position asociada
    for (CommsPositionUpdate const& update : packet.positions) {

        Position* pos = positions_->getPosition(update.entityName);
        if (!pos) {
            SYS_WARN("CommsCore","Unknown position '" + update.entityName + "'");
            continue;
        }

        std::vector<unsigned long long> tx_ids;
        tx_ids.reserve(update.tx.size());
        for (CommsTxSlot const& tx : update.tx)
            tx_ids.push_back(tx.id);
        pos->setTx(tx_ids);

        std::vector<std::pair<unsigned long long, short>> rx_pairs;
        rx_pairs.reserve(update.rx.size());
        for (CommsRxSlot const& rx : update.rx)
            rx_pairs.push_back({ rx.id, static_cast<short>(rx.vol) });
        pos->setRx(rx_pairs);

        pos->set_spk_vol(static_cast<short>(update.speaker_vol));
        pos->set_vox_level(update.vox_level);
        pos->set_mic_enabled(update.mic_enabled);

        // #TODO: disparar el enrutado de audio (TotalMix/Symetrix) en función del nuevo estado
    }

    return true;
}
