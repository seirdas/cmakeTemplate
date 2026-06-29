#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

JsonMgr& JsonMgr::instance() {
    static JsonMgr instance;
    return instance;
}


#if defined JSON || defined JSON_VERSION
    #include <string>

    // Gestión de archivo ----------------------------------------------------------------------

    json* JsonMgr::load(std::string const& filename) {
        std::unique_lock<std::mutex> lock(mtx_);

        // Abrir archivo
        std::ifstream file(filename, std::ios::in);
        if (!file.is_open()) {
            // Crear archivo vacío y meter objeto vacío en caché
            SYS_WARN("JsonMgr", "Cannot open " + filename + ". Creating empty file...");
            std::ofstream newFile(filename, std::ios::out);
            if (!newFile.is_open()) {
                SYS_ERROR("JsonMgr", "Cannot create " + filename + ".");
                return nullptr;
            }
            newFile << json::object().dump(4);
            cache_[filename] = json::object();
            snapshot_[filename] = json::object();
            return &cache_[filename];
        }

        // Leer archivo y "almacenar" en variable json
        json j;
        try {
            // La función parse lanza una excepción si el formato es incorrecto
            j = json::parse(file);
        } catch (const json::parse_error& e) {
            // Captura específicamente errores de formato JSON
            SYS_ERROR("JsonMgr", "Failed to parse " + filename + ": " + e.what());
            return nullptr; 
        } catch (const std::exception& e) {
            // Captura cualquier otro error genérico
            SYS_ERROR("JsonMgr", "Unexpected error parsing " + filename + ": " + e.what());
            return nullptr;
        }

        // Cerrar archivo (opcional)
        if (!file.is_open())
            file.close();

        // Como todo ha ido bien, guardar el json leído en la caché y en snapshot
        cache_[filename] = std::move(j);
        snapshot_[filename] = cache_[filename];

        // Devolver unique_ptr json
        return &cache_[filename];
    }

    bool JsonMgr::update(std::string const& filename) {

        std::unique_lock<std::mutex> lock(mtx_);

        auto itCache = cache_.find(filename);
        auto itSnap  = snapshot_.find(filename);

        // Si no existe el archivo en caché, no hacer nada
        if (itCache == cache_.end())
            return false;

        // Si no hay cambios no hacer nada
        if (itSnap->second == itCache->second)
            return true;

        SYS_INFO("JsonMgr", "Saving config changes to file...");
        
        // Abrir archivo para escritura
        std::ofstream file(filename, std::ios::out);
        if (!file.is_open()) {
            SYS_WARN("JsonMgr", "Failed to save configuration to " + filename);
            return false;
        }

        // Escribir los datos en el archivo
        file << itCache->second.dump(4);
        SYS_INFO("JsonMgr", "Configuration successfully saved.");

        // Guardar los datos actuales en snapshot
        snapshot_[filename] = itCache->second;

        return true;
    }

    json* JsonMgr::getSubNode(std::string const& filename, std::string const& key) {
        std::unique_lock<std::mutex> lock(mtx_);

        auto it = cache_.find(filename);
        if (it == cache_.end()) {
            SYS_WARN("JsonMgr", filename + " not loaded.");
            return nullptr;
        }

        json& root = it->second;
        if (!root.contains(key) || !root[key].is_object())
            root[key] = json::object();

        return &root[key];
    }

    // Lectura de datos ---------------------------------------------------------------------

    /* Implementado en el hpp por los templates */



#else 
// ============================================================
//  (Stubs)
// ============================================================

// Gestión de archivo -------------------------------------------------------------------
    json* JsonMgr::load(std::string const&)     { return nullptr; }
    bool  JsonMgr::update(std::string const&)   { return false; }
    json* JsonMgr::getSubNode(std::string const&, std::string const&) { return nullptr; }

#endif