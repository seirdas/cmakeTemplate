#include "positions/PositionsMgr.hpp"
#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"

#include <string>


// General ------------------------------------------------------------------------------

PositionsMgr::PositionsMgr() :
    initialized_(false)
{

}

PositionsMgr::~PositionsMgr() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool PositionsMgr::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("PositionsMgr","Cannot load config. Using default values.");


    // #TODO

    
    SYS_WARN("PositionsMgr","Comms logic not yet fully implemented");
    
    initialized_ = true;
    return initialized_;    //<- true
}

bool PositionsMgr::isInitialized() const {
    return initialized_;
}

void PositionsMgr::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO
    /*
    
    // Inicializar cada Position definida en el json
    std::string name;
    std::vector<json*> config_Positions = jsonMgr.getArrayElements(cfg, "positions");
    for (json* const cfg_node : config_Positions) {
        name = "";
        std::unique_ptr<Position> pers = std::make_unique<Position>();

        // Obtener el nombre desde aquí (puesto 'alias' para que salga lo primero)
        jsonMgr.get(cfg_node, "alias", name);

        // Inicializar la Position con los datos de la configuración
        if(!name.empty() && !pers->init(cfg_node)) {
            SYS_WARN("PositionsMgr","Cannot initialize new position '" + name + "'");
            continue;
        }

        // Agregar Position creada a la lista de Positions
        Positions_[name] = std::move(pers);
    }
    */


}

bool PositionsMgr::close() {
    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;


    initialized_ = false;
    return !initialized_;   // <- true
}
