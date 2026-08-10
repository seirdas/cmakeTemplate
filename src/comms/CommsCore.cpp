#include "comms/CommsCore.hpp"
#include "comms/Persona.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"
#include <memory>

// General ------------------------------------------------------------------------------

CommsCore::CommsCore(IAppControl* ctrl) :
    initialized_(false),
    ctrl_(ctrl),
    last_packet_hash_(0)
{

}

CommsCore::~CommsCore() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool CommsCore::init(void* config) {

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

bool CommsCore::isInitialized() const {
    return initialized_;
}

void CommsCore::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();
    
    // Inicializar cada persona definida en el json
    std::string name;
    std::vector<json*> config_personas = jsonMgr.getArrayElements(cfg, "positions");
    for (json* const cfg_node : config_personas) {
        name = "";
        std::unique_ptr<Persona> pers = std::make_unique<Persona>();

        // Obtener el nombre desde aquí (puesto 'alias' para que salga lo primero)
        jsonMgr.get(cfg_node, "alias", name);

        // Inicializar la persona con los datos de la configuración
        if(!name.empty() && !pers->init(cfg_node)) {
            SYS_WARN("CommsCore","Cannot initialize new position '" + name + "'");
            continue;
        }

        // Agregar persona creada a la lista de personas
        personas_[name] = std::move(pers);
    }

}

bool CommsCore::close() {
    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;


    initialized_ = false;
    return !initialized_;   // <- true
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
