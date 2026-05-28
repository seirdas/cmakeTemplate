
#pragma once

#include <thread>               // Hilos

#include <nlohmann/json.hpp>    // Manipula archivos .json

#include "gui/GuiMgr.hpp"          // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "sound/TTSMgr.hpp"     // Clase para gestionar TTS
#include "devices/Totalmix.hpp" // Clase para gestionar driver TotalmixFX
#include "devices/Symetrix.hpp" // Clase para gestionar driver TotalmixFX

#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

#define VERSION 0.9

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

    /**
     * @brief Inicializa los miembros de la aplicación
     * @param argc Número de parámetros de entrada del programa
     * @param argv Array de nombres de parámetros de entrada
     */
    bool init(int argc, char** argv);

    /**
     * @brief Ejecuta la aplicación. Inicia el receptor UDP y la ventana UI.
     * @return 0 si todo se ejecutó correctamente, otro en caso de error.
     */
    int run();


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
     * @brief Devuelve la lista con los dispositivos de entrada disponibles.
     */
    std::vector<std::string> getAvailableInputDevices() noexcept override;

    /**
     * @brief Devuelve la lista con los dispositivos de reproducción disponibles.
     */
    std::vector<std::string> getAvailablePlaybackDevices() noexcept override;
    

    // TTS ----------------------------------------------------------------------------

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
     * @brief Obtiene los datos de módulo TTS
     */
    TTSData getTTSData() noexcept override; 

    /**
     * @brief Obtiene el texto procesado por un modelo TTS 
     */
    std::string getTTSProcessingText(std::string modelName) const noexcept override;

    
    // Añadir aquí métodos de IAppControl...

private:

    /************ Variables ********************************************************/

    // Parámetros de entrada (igual que main)
    int         argc_;                          // Número de parámetros de entrada
    char**      argv_;                          // Texto de parámetro de entrada

    // Módulos
    NetMgr      net_;                           // Gestor de sockets de red
    GuiMgr      gui_;                           // Gestor de ventanas para la interfaz gráfica
    SoundMgr    snd_;                           // Gestor de audio
    TTSMgr      tts_;                           // Gestor módulo TTS
    Totalmix    tmx_;                           // Gestor módulo Totalmix
    Symetrix    sym_;                           // Gestor módulo Symetrix

    bool        net_initialized_;               // Indica si net está inicializado
    bool        gui_initialized_;               // Indica si gui está inicializado
    bool        snd_initialized_;               // Indica si snd está inicializado
    bool        tts_initialized_;               // Indica si tts está inicializado

    // Gestión de hilos
    std::thread             worker_;            // gestor de paquetes
    std::mutex              online_mtx_;        // Mutex para dejar en espera al hilo
    std::condition_variable online_cv_;         // Reacciona al cambio de estado para el hilo consumidor

    // Estado de aplicación
    std::atomic<bool>       running_;           // flag de aplicación corriendo (para hilos)
    std::atomic<bool>       online_mode_;       // Modo Online (gestionar paquetes de socket) o offline (ejecuta desde UI)
    std::string             version_;           // Versión de la aplicación
};
