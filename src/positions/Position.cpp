#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// General ------------------------------------------------------------------------------

Position::Position() :
    initialized_(false),
    name_(""),
    is_instructor_(false),
    spk_vol(0),
    tm_({}),
    symIDs_({})
{
    
}

Position::~Position() {

}


// Inicialización y ejecución -----------------------------------------------------------

bool Position::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);

    initialized_ = true;
    return initialized_;
}

bool Position::isInitialized() const {
    return initialized_;
}

void Position::loadConfig(void* config) {
    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    jsonMgr.get_or_set(cfg,"alias",name_);
    jsonMgr.get_or_set(cfg,"is_instructor",is_instructor_);

    // Variable para los nodos
    json* config_node = nullptr;

    // Nodo de totalmix
    {
        config_node = nullptr;
        config_node = jsonMgr.getSubNode(cfg,"totalmix");

        // Valor para ir metiendo en los vectores de in/out
        unsigned short value = 0;

        value = 0;
        jsonMgr.get_or_set(config_node,"in",value);
        if (value != 0) tm_.in.push_back(value);

        value = 0;
        jsonMgr.get_or_set(config_node,"out",value);
        if (value != 0) tm_.out.push_back(value);

        jsonMgr.get_or_set(config_node,"fx",tm_.fx);
        jsonMgr.get_or_set(config_node,"spk_out",tm_.out_spk);
    }

    // Nodo de symetrix
    {
        config_node = nullptr;
        config_node = jsonMgr.getSubNode(cfg,"symetrix");

        jsonMgr.get_or_set(config_node,"supermatrix_in",symIDs_.sm_in);
        jsonMgr.get_or_set(config_node,"supermatrix_out",symIDs_.sm_out);
        jsonMgr.get_or_set(config_node,"id_in_input_sel",symIDs_.in_input_sel);
        jsonMgr.get_or_set(config_node,"id_in_vox_threshold",symIDs_.in_vox_threshold);
        jsonMgr.get_or_set(config_node,"id_in_fx_input_selector",symIDs_.in_fx_input_sel);
        jsonMgr.get_or_set(config_node,"id_out_gain_fader",symIDs_.out_gain_fader);
        jsonMgr.get_or_set(config_node,"id_out_gain_mute",symIDs_.out_gain_mute);
    }

}


// Configuración ------------------------------------------------------------------------

bool Position::set_mic_in(short in_index) {
    // #TODO
    return false;
}

void Position::set_spk_vol(short vol) {
    spk_vol = vol;
}

void Position::set_vox_level(unsigned int level) {
    vox_level_ = level;
}

void Position::set_mic_enabled(bool enabled) {
    mic_enabled_ = enabled;
}


// Transmisión / Recepción ----------------------------------------------------------------

void Position::setTx(std::vector<unsigned long long> const& ids) {
    TXs.clear();
    TXs.reserve(ids.size());
    for (unsigned long long id : ids)
        TXs.push_back({ id });
}

void Position::setRx(std::vector<std::pair<unsigned long long, short>> const& rx) {
    RXs.clear();
    RXs.reserve(rx.size());
    for (auto const& [id, vol] : rx)
        RXs.push_back({ id, vol });
}


// Información --------------------------------------------------------------------------

void Position::logInfo() {
    SYS_INFO("Position","Not yet implemented.");
    return;
}

std::string Position::name() {
    return name_;
}

bool Position::isTransmiting() {
    return !TXs.empty();
}

bool Position::isReceiving() {
    return !RXs.empty();
}
