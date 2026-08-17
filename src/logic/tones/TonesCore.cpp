#include "logic/tones/TonesCore.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

TonesCore::TonesCore(IAppControl* ctrl) :
    initialized_(false),
    ctrl_(ctrl),
    last_packet_hash_(0)
{

}

TonesCore::~TonesCore() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool TonesCore::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("CommsCore","Cannot load config. Using default values.");


    // #TODO

    
    SYS_WARN("CommsCore","Comms logic not yet fully implemented");
    
    initialized_ = true;
    return initialized_;    //<- true
}

bool TonesCore::isInitialized() const {
    return initialized_;
}

void TonesCore::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO

}

bool TonesCore::close() {

    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;

    // #TODO

    initialized_ = false;
    return !initialized_;   // <- true
}

