#include "dispatchers/TonesDispatcher.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

TonesDispatcher::TonesDispatcher() :
    initialized_(false),
    ctrl_(nullptr),
    last_packet_hash_(0)
{

}

TonesDispatcher::~TonesDispatcher() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool TonesDispatcher::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("TonesDispatcher","Cannot load config. Using default values.");


    // #TODO

    
    SYS_WARN("TonesDispatcher","Comms logic not yet fully implemented");
    
    initialized_ = true;
    return initialized_;    //<- true
}

bool TonesDispatcher::isInitialized() const {
    return initialized_;
}

void TonesDispatcher::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO

}

bool TonesDispatcher::close() {

    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;

    // #TODO

    initialized_ = false;
    return !initialized_;   // <- true
}

