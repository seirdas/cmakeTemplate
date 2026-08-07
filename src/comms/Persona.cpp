#include "comms/Persona.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"
#include <cstring>


// General ------------------------------------------------------------------------------

Persona::Persona() :
    initialized_(false),
    name_(""),
    is_instructor_(false),
    spk_vol(0)
{
    memset(&symIDs_, 0, sizeof(symIDs_));
    memset(&tm_, 0, sizeof(tm_));
}

Persona::~Persona() {

}


// Inicialización y ejecución -----------------------------------------------------------

bool Persona::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);

    initialized_ = true;
    return initialized_;
}

bool Persona::isInitialized() const {
    return initialized_;
}

void Persona::loadConfig(void* config) {
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

bool Persona::set_mic_in(short in_index) {
    // #TODO
}

void Persona::set_spk_vol(short vol) {
    spk_vol = vol;
}


// Información --------------------------------------------------------------------------

void Persona::logInfo() {
    SYS_INFO("Persona","Not yet implemented.");
    return;
}

std::string Persona::name() {
    return name_;
}

bool Persona::isTransmiting() {
    return !TXs.empty();
}

bool Persona::isReceiving() {
    return !RXs.empty();
}