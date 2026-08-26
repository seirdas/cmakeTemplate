#include "dispatchers/CommsDispatcher.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"
#include "positions/PositionsMgr.hpp"
#include "positions/Position.hpp"


// General ------------------------------------------------------------------------------

CommsDispatcher::CommsDispatcher() :
    initialized_(false),
    ctrl_(nullptr),
    last_packet_hash_(0)
{

}

CommsDispatcher::~CommsDispatcher() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool CommsDispatcher::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("CommsDispatcher","Cannot load config. Using default values.");


    // #TODO

    
    SYS_WARN("CommsDispatcher","Comms logic not yet fully implemented");
    
    initialized_ = true;
    return initialized_;    //<- true
}

bool CommsDispatcher::isInitialized() const {
    return initialized_;
}

void CommsDispatcher::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO
}

bool CommsDispatcher::close() {
    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;


    initialized_ = false;
    return !initialized_;   // <- true
}


// Ejecución ----------------------------------------------------------------------------

bool CommsDispatcher::Dispatch(std::vector<char> data) {
    // Validar que no venga vacío
    if (data.empty()) 
        return false;

    // Comparar el hash del nuevo paquete con el procesado justo antes
    std::string_view sv(data.data(), data.size());
    size_t hash_actual = std::hash<std::string_view>{}(sv);
    if (hash_actual == last_packet_hash_)
        return false;
    last_packet_hash_ = hash_actual;


    // vvvvvvvv La lógica de comms aquí vvvvvvvvv
    
    /* #TODO */

    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


    return true;
}
