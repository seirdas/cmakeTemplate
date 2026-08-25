#pragma once

#include <string>
#include <vector>
#include <unordered_map>


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
class AudioPlaybackModule;


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
    ConsoleMgr(IAppControl* ctrl = nullptr, std::string const& exePath = "");
    
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
     * @param force Fuerza a lanzar de nuevo la consola aunque haya sido lanzada previamente
     */
    bool LaunchConsole(bool force = false);

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
     * @param command Línea de comando completa introducida por el usuario.
     */
    void execute_cmd(std::string const& command);

    /**
     * @brief Subcomando raíz "sounds": deriva a devices/players según el subcomando.
     */
    void execute_cmd_snd(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomando "sounds devices"
     */
    void execute_cmd_snd_devices(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomando "sounds players": deriva a audio/morse/tts según la categoría.
     */
    void execute_cmd_snd_players(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos de "sounds players morse"
     */
    void execute_cmd_snd_morse(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos de "sounds players audio"
     */
    void execute_cmd_snd_audio(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos de "sounds players tts"
     */
    void execute_cmd_snd_tts(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos comunes de reproducción, compartidos por los distintos
     *  tipos de reproductor (audio, morse, tts): stop, volume, modulevolume,
     *  pitch, channel, isplaying.
     * @param mod Puntero al módulo de reproducción (base común) sobre el que actuar.
     * @param tokens Tokens de la línea completa.
     * @param subIdx Índice dentro de @p tokens donde está el subcomando (stop/volume/...).
     * @return @c true si el subcomando se reconoció y gestionó, @c false si no
     *  coincide con ninguno (el caller decide qué hacer, p.ej. imprimir error).
     */
    bool execute_playback_command(
        AudioPlaybackModule*             mod,
        std::vector<std::string> const&  tokens,
        size_t                           subIdx);

    /**
     * @brief Subcomandos del comando "totalmix"
     * @param tokens Tokens de la línea completa, tokens[0] == "totalmix".
     */
    void execute_totalmix_command(std::vector<std::string> const& tokens);

    /**
     * @brief Subcomandos del comando "symetrix"
     * @param tokens Tokens de la línea completa, tokens[0] == "symetrix".
     */
    void execute_symetrix_command(std::vector<std::string> const& tokens);


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
     * @brief Devuelve el token en la posición @p idx, o cadena vacía si no existe.
     * @details Evita comprobar el tamaño de @p tokens manualmente antes de cada acceso.
     * @param tokens Vector de palabras divididas
     * @param idx Índice del token a obtener
     * @return El token, o "" si @p idx está fuera de rango
     */
    std::string get_token(std::vector<std::string> const& tokens, size_t idx);

    /**
     * @brief Extrae pares --flag valor de una lista de tokens
     * @param tokens Vector de palabras divididas
     * @param size 
     * @return 
     */
    std::unordered_map<std::string, std::string> parse_flags(
        std::vector<std::string> const& tokens, size_t size);


// Imprimir texto -----------------------------------------------------------------------

    /**
     * @brief Función auxiliar para imprimir texto por consola
     * @param txt Texto a imprimir
     */
    void print(std::string const& txt);

    /**
     * @brief Función auxiliar para imprimir texto de error por pantalla
     * @param txt Texto a imprimir
     */
    void print_error(std::string const& txt);


/************ Variables ********************************************************/

// Inicialización y ejecución
    void*           config_;            ///< Configuración del módulo (considerado json)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    bool            initialized_;       ///< Bandera para indicar inicialización exitosa
    bool            running_;           ///< Indica si la ventana se ha cerrado para evitar cerrar varias veces

// Parámetros de la consola
    std::string     AppName_;           ///< Nombre de la aplicación/ventana
    std::string     exe_path_;          ///< Ruta de aplicación (para duplicar app en terminal con Linux)
    bool            tried_to_launch_console_;  ///< Indica si ya se ha indtentado lanzar la consola

};
