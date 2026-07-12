#include "comms/CommsCore.hpp"
#include "comms/Persona.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"

// General ------------------------------------------------------------------------------

CommsCore::CommsCore(IAppControl* ctrl_) {

}

CommsCore::~CommsCore() {

}


// Inicialización -----------------------------------------------------------------------

bool CommsCore::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("CommsCore","Cannot load config. Using default values.");


    // #TODO
    SYS_WARN("CommsCore","Comms logic not yet implemented");
    return false;
}

void CommsCore::loadConfig(void* config) {
    if (!config)
        return;

    SYS_INFO("CommsCore","Reading config node...");

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();
    
    // #TODO...

    SYS_INFO("CommsCore","Config node read OK");

}



// Ejecución ----------------------------------------------------------------------------

bool CommsCore::Ejecutar(std::vector<char> data) {
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
