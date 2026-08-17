#include "logic/tones/TonesCore.hpp"
#include "files/JsonMgr.hpp"

// General ------------------------------------------------------------------------------

TonesCore::TonesCore() :
    initialized_(false)
{

}

TonesCore::~TonesCore() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool TonesCore::init(void* config) {

    return false;
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

    return false;
}

