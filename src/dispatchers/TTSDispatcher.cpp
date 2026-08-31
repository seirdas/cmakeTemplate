#include "dispatchers/TTSDispatcher.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "dispatchers/TTSPacket.hpp"
#include <functional>


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

    return true;
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

    return true;
}

// Ejecución ----------------------------------------------------------------------------

bool TTSDispatcher::Dispatch(TTSPacket* data) {
    if (!data)
        return false;

    // Deduplicar (el mismo paquete puede llegar repetido por iComm)
    std::string key = data->entityName + "|" + data->texto + "|" + data->lang;
    std::size_t hash_actual = std::hash<std::string>{}(key);
    if (hash_actual == last_packet_hash_)
        return false;
    last_packet_hash_ = static_cast<unsigned long>(hash_actual);

    // #TODO: reenviar el paquete al motor de síntesis (ver sound/TTSCore.hpp, sound/PlayerTTS.cpp)
    SYS_INFO("TTSDispatcher","TTS packet from '" + data->entityName + "' (" + data->lang + "): " + data->texto);

    return true;
}
