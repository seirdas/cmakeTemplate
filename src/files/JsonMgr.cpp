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

    bool JsonMgr::update() {

        // Marca si todos los json se han volcado correctamente
        bool allSuccess = true;

        std::unique_lock<std::mutex> lock(mtx_);
        for (auto const& [filename, cacheContent] : cache_) {

            // Buscar el snapshot asociado
            auto itSnap = snapshot_.find(filename);

            // Si existe en snapshot y no hay cambios, nos lo saltamos
            if (itSnap != snapshot_.end() && itSnap->second == cacheContent)
                continue; 

            SYS_INFO("JsonMgr", "Saving config changes to ..." + filename);
            
            // Abrir archivo para escritura
            std::ofstream file(filename, std::ios::out);
            if (!file.is_open()) {
                SYS_WARN("JsonMgr", "Failed to save configuration to " + filename);
                allSuccess = false;
                return false;
            }

            // Escribir los datos en el archivo
            file << cacheContent.dump(4);
            SYS_INFO("JsonMgr", "Configuration successfully saved.");

            // Guardar los datos actuales en snapshot
            snapshot_[filename] = cacheContent;
        }
        return true;
    }

    json* JsonMgr::getSubNode(std::string const& filename, std::string const& key) {
        std::unique_lock<std::mutex> lock(mtx_);

        // busca archivo en cache
        auto it = cache_.find(filename);
        if (it == cache_.end()) {
            SYS_WARN("JsonMgr", filename + " not loaded.");
            return nullptr;
        }

        // crea el sub-nodo si no existe
        json& root = it->second;
        if (!root.contains(key) || !root[key].is_object())
            root[key] = json::object();

        return &root[key];
    }

    json* JsonMgr::getSubNode(json* parent, std::string const& key) {
        // Protección por si nos pasan un padre inválido
        if (!parent) return nullptr;

        // Nota: Si usas hilos concurrentes modificando el MISMO json a la vez,
        // podrías querer proteger esto con std::unique_lock<std::mutex> lock(mtx_);
        // Pero si solo se inicializa al arrancar en el hilo principal, no es crítico.
        std::unique_lock<std::mutex> lock(mtx_);

        // crea el sub-nodo si no existe
        if (!parent->contains(key) || !(*parent)[key].is_object()) {
            (*parent)[key] = json::object();
        }

        return &(*parent)[key];
    }

    std::vector<json*> JsonMgr::getArrayElements(json* parent, const std::string& key) {
        std::vector<json*> resultado;

        // Lo devuelve vacio si no cumple requisitos
        if (parent == nullptr && !parent->contains(key))
            return resultado;

        // Push de elementos en el vector
        auto& elemento = (*parent)[key];
        if (elemento.is_array())
            for (auto& item : elemento)
                resultado.push_back(&item);

        // Devuelve el vector
        return resultado;
    }

    
    // Lectura de datos ---------------------------------------------------------------------

    /* Implementado en el hpp por el template */
    //template<typename T>
    //bool JsonMgr::get(json* config, std::string const& key, T& value);

    /* Implementado en el hpp por el template */
    //template<typename T>
    //bool JsonMgr::get_or_set(json* config, std::string const& key, T& value);


#else 
// ============================================================
//  (Stubs)
// ============================================================

// Gestión de archivo -------------------------------------------------------------------
    json* JsonMgr::load(std::string const&)     { return nullptr; }
    bool  JsonMgr::update(std::string const&)   { return false; }
    json* JsonMgr::getSubNode(std::string const&, std::string const&) { return nullptr; }
    json* JsonMgr::getSubNode(json* parent, std::string const& key)   { return nullptr; }

#endif