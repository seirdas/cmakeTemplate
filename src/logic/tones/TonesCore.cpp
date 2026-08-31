#include "logic/tones/TonesCore.hpp"
#include "logic/tones/TonePacket.hpp"
#include "positions/PositionsMgr.hpp"
#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// General ------------------------------------------------------------------------------

TonesCore::TonesCore() :
    positions_(nullptr)
{

}

TonesCore::~TonesCore() {
    close();
}


// Inicialización -----------------------------------------------------------------------

void TonesCore::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO: parámetros propios de la lógica de tonos
}

void TonesCore::setPositionsManager(PositionsMgr* positions) {
    positions_ = positions;
}


// Ejecución ----------------------------------------------------------------------------

bool TonesCore::Dispatch(TonePacket const& packet) {

    // Construir una clave representativa del paquete completo para el dedup
    // (el ICD real manda una foto completa del catálogo a 100Hz)
    std::string key;
    for (ToneState const& tone : packet.tones) {
        key += tone.tone_id + '|';
        key += (tone.enable ? '1' : '0');
        key += std::to_string(tone.vol) + '|';
        key += (tone.loop ? '1' : '0');
        for (ToneVolumeEntry const& vol : tone.volumenes)
            key += vol.entityName + ":" + std::to_string(vol.volume) + ",";
        key += ';';
    }

    if (isDuplicatePacket(key.data(), key.size()))
        return false;

    if (!positions_) {
        SYS_WARN("TonesCore","Dispatch called without a PositionsMgr assigned");
        return false;
    }

    // Negocio: por cada tono activo, validar las posiciones a las que va dirigido
    for (ToneState const& tone : packet.tones) {

        if (!tone.enable)
            continue;

        for (ToneVolumeEntry const& vol : tone.volumenes) {
            if (vol.volume == 0)
                continue;   // esa posición no escucha este tono

            Position* pos = positions_->getPosition(vol.entityName);
            if (!pos) {
                SYS_WARN("TonesCore","Unknown position '" + vol.entityName + "'");
                continue;
            }

            // #TODO: disparar la reproducción real del tono para 'pos'
            //  (ver sound/PlayerMorse.cpp, sound/PlayerAudio.cpp)
            SYS_INFO("TonesCore","Tone '" + tone.tone_id + "' active for '" + vol.entityName + "' at vol " + std::to_string(vol.volume));
        }
    }

    return true;
}
