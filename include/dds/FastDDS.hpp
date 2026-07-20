#pragma once

#include <memory>
#include <string>

// Temporal mientras la implementación esté en el hpp:


class FastDDS {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    FastDDS();

    /**
     * @brief Destructor estándar
     */
    ~FastDDS();

    // Deshabilitamos copia para evitar problemas con la gestión de recursos DDS
    FastDDS(const FastDDS&) = delete;
    FastDDS& operator=(const FastDDS&) = delete;

    // Permitimos movimiento (Move semantics)
    FastDDS(FastDDS&&) noexcept;
    FastDDS& operator=(FastDDS&&) noexcept;


// Inicialización y ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicialización del servidor FastDDS.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);


private:

/************ Variables ****************************************************************/

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;                                    ///< Declaración de estructura PIMPL para no depender de la librería en el header
    std::unique_ptr<Impl>       pimpl_;             ///< Miembros dependientes de la librería externa

// Inicialización
    bool                        initialized_;       ///< Bandera para indicar inicialización exitosa


// Datos de fastdds
    unsigned int                DOMAIN_ID_;
    std::string                 pqos_name_;

};
