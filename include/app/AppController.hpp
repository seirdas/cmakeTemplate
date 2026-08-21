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
class ConsoleMgr;
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


private:


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
    std::unique_ptr<NetMgr>     net_;          ///< Gestor de sockets de red
    std::unique_ptr<ConsoleMgr> cli_;          ///< Gestor de ventanas para interfaz de consola
    std::unique_ptr<GuiMgr>     gui_;          ///< Gestor de ventanas para la interfaz gráfica
    std::unique_ptr<SoundMgr>   snd_;          ///< Gestor de audio
    std::unique_ptr<TotalMix>   tmx_;          ///< Gestor módulo Totalmix
    std::unique_ptr<Symetrix>   sym_;          ///< Gestor módulo Symetrix
    std::unique_ptr<VoIPMgr>    vip_;          ///< Gestor módulos Voiprec / Voipplay
    std::unique_ptr<CommsCore>  com_;          ///< Gestor lógica comunicaciones
    std::unique_ptr<FastDDS>    dds_;          ///< Gestor DDS (FastDDS)
    std::unique_ptr<CycloneDDS> cds_;          ///< Gestor DDS (CycloneDDS)

// Variables de activación de módulos
    bool    enable_net_;                        ///< Variable de activación de red (network)
    bool    enable_cli_;                        ///< Variable de activación de terminal (GUI fallback)
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
