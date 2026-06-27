#pragma once
#include "system/SystemMgr.hpp"

// Comprobar si se puede usar la librería externa de json
#if defined JSON || defined JSON_VERSION
    #include <nlohmann/json.hpp>
#else
    // Definición vacía o "stub" para que el compilador no se queje
    namespace nlohmann { 
        class json {

        }; 
    }
#endif


using json = nlohmann::json;
#include <unordered_map>


/**
 * @class JsonMgr
 * @brief Clase helper singleton para leer y escribir en archivos json usando nlohmann/json
 * @note Esta clase es para gestionar los métodos de la librería externa, 
 *  no provee la implementación de lectura/escritura de json.
 *  Gestiona los retornos de las funciones si la librería no está activa, para que siga compilando
 * @note Para obtener la instancia "temporalmente", usar una de estas dos opciones
 * `JsonMgr& jsonMgr = JsonMgr::instance();`    (directo)
 * `JsonMgr* jsonMgr = &JsonMgr::instance();`   (como puntero)
 */
class JsonMgr {
public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Acceso al Singleton
     * @return Instancia del singleton
     */
    static JsonMgr& instance();

    // Eliminar constructor de copia y operador de asignación
    JsonMgr(const JsonMgr&) = delete;
    JsonMgr& operator=(const JsonMgr&) = delete;


// Gestión de archivo -------------------------------------------------------------------

    /**
     * @brief Carga un archivo para usarlo como json en el código
     * @details Se añade el estado actual a la caché por si hay cambios posteriores
     */
    json* load(std::string const& filename);

    bool save(std::string const& filename, json* new_json, bool force = false);
    
    json* getSubNode(std::string const& filename, std::string const& key);


// Lectura de datos ---------------------------------------------------------------------

    /**
     * @brief Obtiene un valor desde un nodo JSON específico, validando su tipo.
     * * @tparam T Tipo de dato esperado (std::string, int, float, bool, etc.).
     * @param config Puntero al objeto JSON que contiene la clave.
     * @param key Nombre de la clave a buscar.
     * @param value Referencia donde se almacenará el valor si la lectura es exitosa.
     * @return true Si la clave existe y el tipo coincide con T.
     * @return false Si la clave no existe o el tipo es incompatible.
     */
    template<typename T>
    bool get(json* config, std::string const& key, T& value) {
        if (!config || !config->contains(key))
            return false;

        const json& node = (*config)[key];

        // Comprobar tipo en tiempo de ejecución según T
        if constexpr (std::is_same_v<T, std::string>) {
            if (!node.is_string()) return false;
        } else if constexpr (std::is_floating_point_v<T>) {
            if (!node.is_number()) return false;        // acepta int y float del json
        } else if constexpr (std::is_integral_v<T>) {
            if (!node.is_number_integer()) return false;
        } else if constexpr (std::is_same_v<T, bool>) {
            if (!node.is_boolean()) return false;
        }

        value = node.get<T>();
        return true;
    }

    /**
     * @brief Intenta obtener un valor; si no existe, lo crea en el JSON con el valor por defecto.
     * * @details Esta función es ideal para inicializar configuraciones. Si la clave no está 
     * presente en el JSON, escribe el valor proporcionado en `value` dentro del objeto JSON,
     * permitiendo que los archivos de configuración se auto-rellenen.
     * * @tparam T Tipo de dato.
     * @param config Puntero al objeto JSON.
     * @param key Nombre de la clave.
     * @param value Valor por defecto (si no se encuentra la clave) o valor obtenido.
     * @return true Si se leyó correctamente del JSON.
     * @return false Si no existía y se ha procedido a escribir el valor por defecto.
     */
    template<typename T>
    bool get_or_set(json* config, std::string const& key, T& value) {
        if (get<T>(config, key, value))
            return true;        // leído del json

        (*config)[key] = value; // write-back del default
        return false;           // indica que se usó el default
    }


private:

// Otros --------------------------------------------------------------------------------

    JsonMgr() = default;    ///< Constructor privado
    ~JsonMgr() = default;   ///< Destructor privado


    /************ Variables ********************************************************/

    std::mutex mtx_;

    // Cache de archivos json
    std::unordered_map<std::string, json> cache_;       ///< JSON's. Como se pasan como puntero, pueden ser modificados
    std::unordered_map<std::string, json> snapshot_;    ///< Copia del json original, por si se modifica (para el `save`)
};
