#pragma once

#include "app/IModule.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>


// Forward declaration
class Position;         ///< Declaración implícita


/** 
 * @class PositionsMgr
 * @brief Clase de gestión de datos recibidos 
 *  externamente de comunicaciones
 */
class PositionsMgr : public IModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    PositionsMgr();

    /**
     * @brief Destructor estándar
     */
    ~PositionsMgr() override;

    // Sin copia ni movimiento
    PositionsMgr(const PositionsMgr&) = delete;
    PositionsMgr& operator=(const PositionsMgr&) = delete;
    PositionsMgr(PositionsMgr&&) = delete;
    PositionsMgr& operator=(PositionsMgr&&) = delete;


// Métodos comunes de módulo (IModule) --------------------------------------------------

    /**
     * @brief Inicializa a las personas
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config) override;

    /**
     * @brief Cierra la lógica de comunicaciones
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close() override;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config) override;


// Ejecución ----------------------------------------------------------------------------

    bool addPosition();

    bool removePosition();

    /**
     * @brief Busca una posición por su nombre/alias.
     * @param name Nombre/alias de la posición.
     * @return Puntero a la posición, o @c nullptr si no existe.
     */
    Position* getPosition(std::string const& name);

    std::vector<std::string> getPositions();


private:

/************ Variables ********************************************************/

// Aliases
    using PositionList = std::unordered_map<std::string, std::unique_ptr<Position>>;

// Gestión de personas
    PositionList        positions_;      ///< Lista de personas gestionadas
    mutable std::mutex  positions_mtx_;  ///< Protege positions_ entre hilos (Comms/Tones llaman desde hilos distintos)

};
