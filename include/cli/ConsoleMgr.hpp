#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>


// includes de consola según SO
#ifdef _WIN32
    #include <conio.h> // Para _getch()
    #include <windows.h>
#else
    #include <termios.h> 
    #include <unistd.h> 
#endif


// Foward declaration
class IAppControl;


/** 
 * @class ConsoleMgr
 * @brief Clase gestora de consola (fallback de GUI)
 */
class ConsoleMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    ConsoleMgr(IAppControl* ctrl = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ConsoleMgr();
    
    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    ConsoleMgr(ConsoleMgr const&) = delete;
    ConsoleMgr& operator=(ConsoleMgr const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    ConsoleMgr(ConsoleMgr&&) = delete;
    ConsoleMgr& operator=(ConsoleMgr&&) = delete;


// Inicialización -----------------------------------------------------------------------

    /**
     * @brief Inicializa la lógica de reproducción de tonos
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
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


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Bucle bloqueante para permitir entrada
     * de comandos en la terminal
     */
    bool Run();


// Opciones de consola ------------------------------------------------------------------

    /**
     * @brief Lanza la consola. 
     * @details Diferencia sistema Windows y Linux.
     */
    bool LaunchConsole();

    /**
     * @brief Establece el nombre de la ventana de terminal
     * @param title Nombre de ventana
     */
    void setConsoleTitle(std::string const& title);


private:

// Comandos -----------------------------------------------------------------------------

    /**
     * @brief Interpreta y ejecuta un comando de texto recibido por la CLI.
     * @details Tokeniza la línea (respetando comillas) y usa una tabla de dispatch
     *  (nombre de comando -> handler)
     * @param cmd Línea de comando completa introducida por el usuario.
     */
    void execute_command(std::string const& cmd);

    /**
     * @brief Subcomandos del comando "sounds"
     * @param tokens Tokens de la línea completa, tokens[0] == "sounds".
     */
    void execute_sounds_command(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos del comando "symetrix"
     * @param tokens Tokens de la línea completa, tokens[0] == "symetrix".
     */
    void execute_symetrix_command(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos del comando "totalmix"
     * @param tokens Tokens de la línea completa, tokens[0] == "totalmix".
     */
    void execute_totalmix_command(std::vector<std::string> const& tokens);
    

// Tokens (comandos) --------------------------------------------------------------------

    /**
     * @brief Divide una línea de comandos en tokens, respetando comillas dobles
     * @details Obtiene de una línea de palabras, un vector con las palabras divididas
     * @note Respeta las comillas ( "Hola mundo" es solo un token -> Hola mundo)
     * @param line Línea de palabras
     * @return Vector con las palabras divididas
     */
    std::vector<std::string> tokenize_cli(std::string const& line);

    /**
     * @brief Extrae pares --flag valor de una lista de tokens
     * @param tokens Vector de palabras divididas
     * @param size 
     * @return 
     */
    std::unordered_map<std::string, std::string> parse_flags(
        std::vector<std::string> const& tokens, size_t size);


/************ Variables ********************************************************/

// Inicialización y ejecución
    void*           config_;            ///< Configuración del módulo (considerado json)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    bool            initialized_;       ///< Bandera para indicar inicialización exitosa
    bool            running_;           ///< Indica si la ventana se ha cerrado para evitar cerrar varias veces

// Parámetros de la consola
    std::string     AppName_;           ///< Nombre de la aplicación/ventana

};
