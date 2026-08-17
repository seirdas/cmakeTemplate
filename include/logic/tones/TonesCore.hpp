#pragma once

class TonesCore {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    TonesCore();
    
    /**
     * @brief Destructor
     */
    ~TonesCore();


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa la lógica de gestión de tonos
     * @return @c true si la inicialización fue exitosa, @c false si hubo algún error 
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

    /**
     * @brief Cierra la lógica y libera los recursos asociados.
     * @return @c true Si ha cerrado correctamente, @c false en caso de error
     */
    bool close();


private:

/************ Variables ********************************************************/

    bool        initialized_;   ///< Bandera para indicar inicialización exitosa


};
