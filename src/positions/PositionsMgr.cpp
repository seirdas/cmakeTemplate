#include "positions/PositionsMgr.hpp"
#include "app/IModule.hpp"
#include "positions/Position.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"
#include <vector>

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
    // IModule::init() guarda la config y llama a loadConfig(), que es donde se
    // crean las Position a partir del array "positions" del json.
    if (!IModule::init(config))
        return false;

    std::size_t n;
    {
        std::lock_guard<std::mutex> lock(positions_mtx_);
        n = positions_.size();
    }

    if (n == 0)
        SYS_WARN("PositionsMgr", "No positions defined in config");
    else
        SYS_INFO("PositionsMgr", "Initialized " + std::to_string(n) + " position(s)");

    return true;
}


void PositionsMgr::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    
    // Inicializar cada Position definida en el json
    std::vector<json*> config_Positions = jsonMgr.getArrayElements(cfg, "positions");

    std::lock_guard<std::mutex> lock(positions_mtx_);
    for (json* const cfg_node : config_Positions) {
        std::string name;
        jsonMgr.get(cfg_node, "alias", name);

        // Sin 'alias' no hay clave para la lista: se descarta
        if (name.empty()) {
            SYS_WARN("PositionsMgr", "Skipping position without 'alias'");
            continue;
        }

        // Crear e inicializar la Position con los datos de la configuración
        std::unique_ptr<Position> pers = std::make_unique<Position>();
        if (!pers->init(cfg_node)) {
            SYS_WARN("PositionsMgr", "Cannot initialize position '" + name + "'");
            continue;
        }

        // Agregar Position creada a la lista
        positions_[name] = std::move(pers);
        SYS_INFO("PositionsMgr", "Position created: " + name);
    }

}

bool PositionsMgr::close() {
    // Ejecutar cierre común (cambia flags, etc.)
    if (!IModule::close())
        return false;

    return true;
}


// Ejecución ----------------------------------------------------------------------------

Position* PositionsMgr::getPosition(std::string const& name) {
    std::lock_guard<std::mutex> lock(positions_mtx_);

    auto it = positions_.find(name);
    if (it == positions_.end())
        return nullptr;

    return it->second.get();
}

std::vector<std::string> PositionsMgr::getPositions() {
    std::lock_guard<std::mutex> lock(positions_mtx_);

    std::vector<std::string> names;
    names.reserve(positions_.size());
    for (auto const& [name, pos] : positions_)
        names.push_back(name);

    return names;
}
