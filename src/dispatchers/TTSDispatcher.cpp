#include "dispatchers/TTSDispatcher.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "dispatchers/TTSPacket.hpp"


// General ------------------------------------------------------------------------------

TTSDispatcher::TTSDispatcher() :
    last_packet_hash_(0)
{

}

TTSDispatcher::~TTSDispatcher() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool TTSDispatcher::init(void* config) {
    if (!IModule::init(config))
        return false;
}

void TTSDispatcher::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO

}

bool TTSDispatcher::close() {
    if (!IModule::close())
        return false;
}

// Ejecución ----------------------------------------------------------------------------

bool Dispatch() {

    return false;
}
