
#pragma once

#include <thread>               // Hilos
#include <memory>               // unique_ptr
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

// Declaración implícita
class NetMgr;
class GuiMgr;
class SoundMgr;
class TTSMgr;
class TotalMix;
class Symetrix;
class VoIPMgr;
class CommsCore;
class FastDDS;
class CycloneDDS;

/**
  *  @class AppController
  *  @brief Clase principal que coordina los subsistemas de la aplicación.
  *  @details AppController implementa la interfaz IAppControl y actúa como núcleo
  *   de la aplicación, inicializando y gestionando los componentes principales.
  *   Proporciona métodos para inicializar los módulos y 
  *   ejecutar el flujo principal de la aplicación.
  *   Comportamiento:
  *      init() inicializa los miembros necesarios (red, UI, audio, etc.).
  *      run() Mantiene el ciclo de vida de la aplicación hasta su finalización.
  *  @note La variable VERSION se usa para construir version_.
  *  @see IAppControl
  */
class AppController : public IAppControl {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AppController. 
     */
    AppController();

    /**
     * @brief Destructor de AppController.
     */
    ~AppController();

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    AppController(AppController const&) = delete;
    AppController& operator=(AppController const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    AppController(AppController&&) = delete;
    AppController& operator=(AppController&&) = delete;


// Inicialización y ejecución -----------------------------------------------------------

    /**
     * @brief Inicializa los miembros de la aplicación
     * @param argc Número de parámetros de entrada del programa
     * @param argv Array de nombres de parámetros de entrada
     */
    bool init(int argc, char** argv);

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
     * @brief Cierra los módulos y los hilos iniciados para terminar la ejecución
     */
    void close();

    /**
     * @brief Ejecuta la aplicación. Inicia el receptor UDP y la ventana UI.
     * @note BLOQUEANTE hasta que se cierre la GUI o en su defecto la terminal
     * @return @c true si todo se ejecutó correctamente, @c false en caso de error.
     */
    bool run();


// Hilos --------------------------------------------------------------------------------

    /**
     * @brief Hilo gestor de paquetes online
     * @note Solo se ejecuta cuando la aplicación está en online, de lo contrario se queda parado.
     */
    void TWorker();


// IAppControl methods ------------------------------------------------------------------

    // Aplicación -----------------------------------------------------------------------

    /**
     * @brief Implementación del método de IAppControl para devolver la versión de la aplicación.
     * @return La versión de la aplicación como una cadena de texto.
     * @note const evita que el método modifique la variable "version_" o cualquiera.
     */
    std::string getVersion() const noexcept override;

    /**
     * @brief Establece el modo online/offline
     * @param newMode true = online, false = offline
     */
    void setOnlineMode(bool newMode) noexcept override;

    /**
     * @brief Obtiene el estado Online/Offline
     * @returns true = online, false = offline
     */
    bool isOnlineMode() const noexcept override;


    // Sockets --------------------------------------------------------------------------

    /**
     * @brief Agrega un nuevo receptor de datos de red.
     * @return true si el receptor se agregó correctamente, false en caso de error.
     */
    bool addReceiver() const noexcept override;

    /**
     * @brief Elimina un receptor de datos de red.
     * @return true si el receptor se eliminó correctamente, false en caso de error.
     */
    bool removeReceiver() const noexcept override;


    // Audio ----------------------------------------------------------------------------

    /**
     * @brief refresca la lista de dispositivos de entrada
     */
    void refreshAudioDevices() noexcept override;

    /**
     * @brief Devuelve la lista con los dispositivos de entrada disponibles.
     */
    std::vector<std::string> getAvailableInputDevices() noexcept override;

    /**
     * @brief Devuelve una lista con los nombres de todos las capturas agregadas
     */
    std::vector<std::string> getManagedCaptures() noexcept override;
    
    /**
     * @brief Devuelve la lista con los dispositivos de reproducción disponibles.
     */
    std::vector<std::string> getAvailablePlaybackDevices() noexcept override;

    /**
     * @brief Añade un dispositivo de captura por nombre.
     */
    bool addInputDevice(std::string const& captureName, std::string const& deviceName) noexcept override;

    /**
     * @brief Elimina un dispositivo de captura por índice.
     */
    bool removeInputDevice(std::string const& captureName) noexcept override;
    
    /**
     * @brief Manda que se empiece a grabar
     */
    bool StartRecording(std::string const& captureName) noexcept override;

    /**
     * @brief Manda que pare de grabar
     */
    bool StopRecording(std::string const& captureName) noexcept override;

    /**
     * @brief Obtiene el tamaño del buffer de captura
     * @param index indice de dispositivo de captura
     * @return el tamaño del buffer de captura
     */
    size_t getInputBufferSize(std::string const& captureName) noexcept override;

    /**
     * @brief Obtiene el tamaño del buffer de grabación
     * @param index indice de dispositivo de captura
     * @return el tamaño del buffer de grabación
     */
    size_t getInputRecBufferSize(std::string const& captureName) noexcept override;

    /**
     * @brief Comprobar si el dispositivo sigue activo y funcionando
     */
    bool isInputDeviceValid(std::string const& captureName) noexcept override;

    /**
     * @brief devuelve el nivel del pico de audio
     */
    float getInputPeakLevel(std::string const& captureName) noexcept override;

    /**
     * @brief devuelve el nivel del RMS de audio
     */
    float getInputRMSLevel(std::string const& captureName) noexcept override;


    // TTS ----------------------------------------------------------------------------------

    /**
     * @brief Genera un audio a partir de un texto usando el modelo de voz especificado.
     * @param modelName Nombre del modelo que genera el audio
     * @param text El texto a convertir en audio.
     * @param wavname El nombre del archivo WAV de salida (incluyendo la extensión wav).
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool TTSgenerate(
        std::string const& modelName, 
        std::string const& text, 
        std::string const& wavname)
        noexcept override;

    /**
     * @brief Reproduce un audio por un playback genereado mediante un texto, usando TTS
     * @details No genera ningún archivo de audio intermedio; se genera y se reproduce
     * @param modelName Nombre del modelo que genera el audio
     * @param text El texto a convertir en audio.
     * @param playbackName El nombre del playback
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool TTSPlay(
        std::string const& modelName, 
        std::string const& text, 
        std::string const& playbackName)
        noexcept override;


    // VoIP ---------------------------------------------------------------------------------

    bool addVoiprec(
    std::string const&  voiprecName,
    bool                isPlaybackSource,
    std::string const&  audioSourceName,
    const std::string   socketName, 
    std::string const&  ipDest,
    unsigned short      port) noexcept override;
    
    // Añadir aquí métodos de IAppControl...

private:

    // Consola ------------------------------------------------------------------------------

    /**
     * @brief Lanza la consola. 
     * @details Diferencia sistema Windows y Linux.
     */
    bool launch_console();

    /**
     * @brief Buble bloqueante para permitir entrada
     * de comandos en la terminal
     */
    bool run_cli_loop();

    /**
     * @brief Interpreta y ejecuta un comando de texto recibido por la CLI.
     * @details Tokeniza la línea (respetando comillas) y usa una tabla de dispatch
     *  (nombre de comando -> handler)
     * @param cmd Línea de comando completa introducida por el usuario.
     */
    void execute_command(const std::string& cmd);

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



/************ Variables ********************************************************/

// Datos de aplicación
    std::string             app_name_;          ///< Nombre de aplicación     
    std::string             version_;           ///< Versión de la aplicación
    bool                    initialized_;       ///< Bandera para indicar inicialización exitosa

// Inicialización y ejecución
    std::atomic<bool>       running_;           ///< flag de aplicación corriendo (para hilos)
    std::atomic<bool>       online_mode_;       ///< Modo Online (gestionar paquetes de socket) o offline (ejecuta desde UI)
    
// Parámetros de entrada (igual que main)
    int         argc_;                          ///< Número de parámetros de entrada
    char**      argv_;                          ///< Texto de parámetro de entrada

// Archivos de configuración
    std::string             config_filename_;   ///< Nombre de archivo de configuración

// Módulos
    std::unique_ptr<NetMgr>      net_;          ///< Gestor de sockets de red
    std::unique_ptr<GuiMgr>      gui_;          ///< Gestor de ventanas para la interfaz gráfica
    std::unique_ptr<SoundMgr>    snd_;          ///< Gestor de audio
    std::unique_ptr<TotalMix>    tmx_;          ///< Gestor módulo Totalmix
    std::unique_ptr<Symetrix>    sym_;          ///< Gestor módulo Symetrix
    std::unique_ptr<VoIPMgr>     vip_;          ///< Gestor módulos Voiprec / Voipplay
    std::unique_ptr<CommsCore>   com_;          ///< Gestor lógica comunicaciones
    std::unique_ptr<FastDDS>     dds_;          ///< Gestor DDS (FastDDS)
    std::unique_ptr<CycloneDDS>  cds_;          ///< Gestor DDS (CycloneDDS)

// Variables de activación de módulos
    bool    enable_net_;                        ///< Variable de activación de red (network)
    bool    enable_gui_;                        ///< Variable de activación de GUI
    bool    enable_snd_;                        ///< Variable de activación de sonido
    bool    enable_tmx_;                        ///< Variable de activación de Totalmix
    bool    enable_sym_;                        ///< Variable de activación de Symetrix
    bool    enable_vip_;                        ///< Variable de activación de VoIP
    bool    enable_com_;                        ///< Variable de activación de lógica de Comms
    bool    enable_dds_;                        ///< Variable de activación de DDS (FastDDS)
    bool    enable_cds_;                        ///< Variable de activación de DDS (CycloneDDS)

// Gestión de hilos
    std::thread             consumer_thread_;   ///< Hilo consumidor de paquetes de red
    std::mutex              online_mtx_;        ///< Mutex para dejar en espera al hilo
    std::condition_variable online_cv_;         ///< Reacciona al cambio de estado para el hilo consumidor

};
