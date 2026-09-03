#pragma once

#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación
#include <memory>               // unique_ptr
#include <atomic>

// Declaración implícita
class NetMgr;
class GuiMgr;
class PositionsMgr;
class ConsoleMgr;
class SoundMgr;
class TotalMix;
class Symetrix;
class VoIPMgr;
class FastDDS;
class CycloneDDS;
class iCommBridge;
class CommsCore;
class TonesCore;
class TTSDispatcher;


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
     * @param argc Número de parámetros de entrada del programa
     * @param argv Array de nombres de parámetros de entrada
     */
    AppController(int argc, char** argv);

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
     */
    bool init();

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
    bool Run();


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
     * @param newMode @c true = online, @c false = offline
     */
    void setOnlineMode(bool newMode) noexcept override;

    /**
     * @brief Obtiene el estado Online/Offline
     * @returns @c true = online, @c false = offline
     */
    bool isOnlineMode() const noexcept override;


    // Módulos --------------------------------------------------------------------------

    /**
     * @brief Devuelve la instancia de gestor de red
     * @return Puntero a gestor de red
     */
    NetMgr* getNetModule() override;

    /**
     * @brief Devuelve la instancia de gestor de sonidos
     * @return Puntero a gestor de sonidos
     */
    SoundMgr* getSoundsModule() override;

    /**
     * @brief Devuelve la instancia de gestor de Totalmix
     * @return Puntero a gestor de totalmix
     */
    TotalMix* getTotalmixModule() override;

    /**
     * @brief Devuelve la instancia de gestor de Symetrix
     * @return Puntero a gestor de symetrix
     */
    Symetrix* getSymetrixModule() override;

    /**
     * @brief Devuelve la instancia de gestor de FastDDS
     * @return Puntero a gestor de FastDDS
     */
    FastDDS* getFastDDSModule() override;


private:

// Módulos ------------------------------------------------------------------------------

    /**
     * @brief Inicializa un módulo genérico de la aplicación si se encuentra habilitado.
     * 
     * @details Comprueba si el indicador de activación está en true. En caso afirmativo,
     *          obtiene su subnodo de configuración correspondiente desde el gestor JSON 
     *          mediante el nombre especificado, invoca el método @c init() del módulo 
     *          e imprime mensajes informativos o de advertencia en el sistema de logs.
     * 
     * @tparam T Tipo del puntero inteligente o gestor del módulo (ej. std::unique_ptr<NetMgr>).
     * @param module Referencia al puntero del módulo que se desea inicializar.
     * @param name Nombre identificativo del módulo (utilizado para los logs y la búsqueda en el JSON).
     * @param enabled Bandera booleana que indica si el módulo debe ser inicializado o ignorado.
     * 
     * @note Se asume que el tipo @c T expone un operador de desreferencia (->) y un método 
     *       @c init(json*) compatible.
     */
    template <typename T>
    void init_module(T& module, std::string const& name, bool enabled);


    /**
     * @brief Cierra un módulo
     * @tparam T Tipo del puntero inteligente o gestor del módulo (ej. std::unique_ptr<NetMgr>).
     * @param module Referencia al puntero del módulo que se desea inicializar.
     * @param name Nombre identificativo del módulo (utilizado para los logs).
     */
    template <typename T>
    void close_module(T& module, std::string const& name);



/************ Variables ********************************************************/

// Datos de aplicación
    std::string             app_name_;          ///< Nombre de aplicación     
    std::string             version_;           ///< Versión de la aplicación
    bool                    initialized_;       ///< Bandera para indicar inicialización exitosa

// Inicialización y ejecución
    std::atomic<bool>       online_mode_;       ///< Modo online/offline
    
// Parámetros de entrada (igual que main)
    int                     argc_;              ///< Número de parámetros de entrada
    char**                  argv_;              ///< Texto de parámetro de entrada

// Archivos de configuración
    std::string             config_filename_;   ///< Nombre de archivo de configuración

// Módulos
    std::unique_ptr<NetMgr>         net_;          ///< Gestor de sockets de red
    std::unique_ptr<PositionsMgr>   pos_;          ///< Gestor de posiciones (personas)
    std::unique_ptr<ConsoleMgr>     cli_;          ///< Gestor de ventanas para interfaz de consola
    std::unique_ptr<GuiMgr>         gui_;          ///< Gestor de ventanas para la interfaz gráfica
    std::unique_ptr<SoundMgr>       snd_;          ///< Gestor de audio
    std::unique_ptr<TotalMix>       tmx_;          ///< Gestor módulo Totalmix
    std::unique_ptr<Symetrix>       sym_;          ///< Gestor módulo Symetrix
    std::unique_ptr<VoIPMgr>        vip_;          ///< Gestor módulos Voiprec / Voipplay
    std::unique_ptr<FastDDS>        dds_;          ///< Gestor DDS (FastDDS)
    std::unique_ptr<CycloneDDS>     cds_;          ///< Gestor DDS (CycloneDDS)
    std::unique_ptr<iCommBridge>    ico_;          ///< Gestor iComm (TTS)
    std::unique_ptr<CommsCore>      comms_;        ///< Lógica de comunicaciones (paquetes de net/dds/CLI.NET)
    std::unique_ptr<TonesCore>      tones_;        ///< Lógica de tonos (paquetes de net/dds/CLI.NET)
    std::unique_ptr<TTSDispatcher>  tts_;          ///< Lógica de TTS (paquetes de iComm, conectado vía callback a ico_)

// Bits de activación de módulos (Bitfield)
    struct ModuleFlags {
        bool net : 1;
        bool pos : 1;
        bool cli : 1;
        bool gui : 1;
        bool snd : 1;
        bool tmx : 1;
        bool sym : 1;
        bool vip : 1;
        bool com : 1;
        bool tns : 1;   ///< Activa/desactiva TonesCore
        bool dds : 1;
        bool cds : 1;
        bool ico : 1;

        // Método para poner todos al valor deseado
        void setAll(bool val) {
            net = pos = cli = gui = snd = tmx = sym = vip = com = tns = dds = cds = ico = val;
        }
    } enable_flags_;                            ///< Bits de activación de módulos (Bitfield)

};
