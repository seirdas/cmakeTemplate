#include "positions/PositionsMgr.hpp"
#include "app/IModule.hpp"
#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"

#include <string>


// General ------------------------------------------------------------------------------

PositionsMgr::PositionsMgr() :
    IModule()
{

}

PositionsMgr::~PositionsMgr() {
    close();
}


// Métodos comunes de módulo (IModule) --------------------------------------------------

bool PositionsMgr::init(void* config) {
    if (!IModule::init(config))
        return false;
    

    // #TODO

    
    SYS_WARN("PositionsMgr","Comms logic not yet fully implemented");
    
    return true;
}

void PositionsMgr::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    
    // Inicializar cada Position definida en el json
    std::string name;
    std::vector<json*> config_Positions = jsonMgr.getArrayElements(cfg, "positions");
    for (json* const cfg_node : config_Positions) {
        name = "";
        std::unique_ptr<Position> pers = std::make_unique<Position>();

        // Obtener el nombre desde aquí (puesto 'alias' para que salga lo primero)
        jsonMgr.get(cfg_node, "alias", name);

        // Sin 'alias' no hay clave para la lista: se descarta
        if (name.empty()) {
            SYS_WARN("PositionsMgr","Skipping position without 'alias'");
            continue;
        }

        // Inicializar la Position con los datos de la configuración
        if (!pers->init(cfg_node)) {
            SYS_WARN("PositionsMgr","Cannot initialize new position '" + name + "'");
            continue;
        }

        // Agregar Position creada a la lista de Positions
        positions_[name] = std::move(pers);
    }

}

bool PositionsMgr::close() {
    // Ejecutar cierre común (cambia flags, etc.)
    if (!IModule::close())
        return false;

    return true;
}
