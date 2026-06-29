#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

// Comprobar si se puede usar la librería externa de json
#if defined JSON || defined JSON_VERSION
    #include <nlohmann/json.hpp>
#else
    // Definición vacía o "stub" para que el compilador no se queje
    namespace nlohmann { class json {}; }
#endif
using json = nlohmann::json;


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
     * @brief Carga un archivo JSON desde el disco y lo almacena en la caché interna.
     * @details Si el archivo no existe, crea un nuevo archivo JSON vacío en la ruta especificada.
     * Los datos cargados se mantienen en memoria dentro de un @c std::unordered_map para
     * acceso rápido.
     * @param filename Ruta del archivo JSON a cargar.
     * @return Puntero al objeto @c json contenido en la caché, o @c nullptr si el parseo falla.
     * @note Si no encuentra el archivo, intentará crearlo.
     */
    json* load(std::string const& filename);

    /**
     * @brief Sincroniza los cambios en memoria con el archivo físico en disco.
     * @details Compara el estado actual (caché) con el último snapshot guardado. Si existen
     * diferencias o se fuerza la actualización, escribe todo el contenido al archivo.
     * @param filename Ruta del archivo a actualizar.
     * @param force Si es @c true, omite la comprobación de cambios y fuerza la escritura a disco.
     * @return @c true si la operación se realizó con éxito o no hubo cambios necesarios, 
     * @c false si hubo error de escritura o el archivo no está en caché.
     */
    bool update(std::string const& filename);
    
    /**
     * @brief Obtiene un sub-nodo específico dentro del objeto JSON raíz.
     * @details Si el nodo solicitado no existe, esta función lo crea automáticamente como un
     * objeto vacío (@c json::object()) dentro de la estructura raíz para facilitar la
     * inicialización jerárquica de módulos.
     * @param filename Nombre del archivo asociado (debe estar previamente cargado en la caché).
     * @param key Clave o nombre del sub-nodo a recuperar.
     * @return Puntero al sub-nodo @c json solicitado, o @c nullptr si el archivo no está cargado.
     */
    json* getSubNode(std::string const& filename, std::string const& key);


// Lectura de datos ---------------------------------------------------------------------

    /**
     * @brief Obtiene un valor desde un nodo JSON específico, validando su tipo.
     * @tparam T Tipo de dato esperado (std::string, int, float, bool, etc.).
     * @param config Puntero al objeto JSON que contiene la clave.
     * @param key Nombre de la clave a buscar.
     * @param value Referencia donde se almacenará el valor si la lectura es exitosa.
     * @return true Si la clave existe y el tipo coincide con T.
     * @return false Si la clave no existe o el tipo es incompatible.
     */
    template<typename T>
    bool get(json* config, std::string const& key, T& value);

    /**
     * @brief Intenta obtener un valor; si no existe, lo crea en el JSON con el valor por defecto.
     * @details Esta función es ideal para inicializar configuraciones. Si la clave no está 
     * presente en el JSON, escribe el valor proporcionado en `value` dentro del objeto JSON,
     * permitiendo que los archivos de configuración se auto-rellenen.
     * @tparam T Tipo de dato.
     * @param config Puntero al objeto JSON.
     * @param key Nombre de la clave.
     * @param value Valor por defecto (si no se encuentra la clave) o valor obtenido.
     * @return true Si se leyó correctamente del JSON.
     * @return false Si no existía y se ha procedido a escribir el valor por defecto.
     */
    template<typename T>
    bool get_or_set(json* config, std::string const& key, T& value);


private:

// Otros --------------------------------------------------------------------------------

    JsonMgr() = default;    ///< Constructor privado
    ~JsonMgr() = default;   ///< Destructor privado


    /************ Variables ********************************************************/

    using jsonMap = std::unordered_map<std::string, json>;
    
    // Cache de archivos json
    jsonMap cache_;         ///< JSON's. Como se pasan como puntero, pueden ser modificados
    jsonMap snapshot_;      ///< Copia del json original, por si se modifica (para el `save`)
    std::mutex mtx_;        ///< Mutex de acceso al json
};


// ==============================================================================
// Implementación de Templates
// ==============================================================================

#if defined JSON || defined JSON_VERSION

// Lectura de datos ---------------------------------------------------------------------

    template<typename T>
    bool JsonMgr::get(json* config, std::string const& key, T& value) {
        if (!config || !config->contains(key))
            return false;

        const json& node = (*config)[key];

        if constexpr (std::is_same_v<T, bool>)
            if (!node.is_boolean()) 
                return false;
        else if constexpr (std::is_same_v<T, std::string>)
            if (!node.is_string()) 
                return false;
        else if constexpr (std::is_floating_point_v<T>)
            if (!node.is_number()) 
                return false;
        else if constexpr (std::is_integral_v<T>)   // <- también entraría con bool
            if (!node.is_number_integer()) 
                return false;

        value = node.get<T>();
        return true;
    }

    template<typename T>
    bool JsonMgr::get_or_set(json* config, std::string const& key, T& value) {
        if (get<T>(config, key, value))
            return true;

        (*config)[key] = value; 
        return false;           
    }

#else

// Lectura de datos ---------------------------------------------------------------------

    template<typename T>
    bool JsonMgr::get(json* config, std::string const& key, T& value)  { return false; }

    template<typename T>
    bool JsonMgr::get_or_set(json* config, std::string const& key, T& value) { return false; }

#endif