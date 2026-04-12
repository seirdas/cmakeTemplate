
#pragma once

#include <thread>               // Hilos

#include <nlohmann/json.hpp>    // Manipula archivos .json

#include "ui/UiMgr.h"           // Clase de gestión de ventana UI
#include "net/NetMgr.hpp"       // Clase para gestionar sockets
#include "sound/SoundMgr.hpp"   // Clase para gestionar audio
#include "sound/TTSMgr.hpp"     // Clase para gestionar TTS
#include "system/LogMgr.hpp"    // Clase para gestionar logs
#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

#define VERSION 0.8

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
  *  @author
  *  @see IAppControl
  *  @date March 2, 2026
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
     */
    bool init();

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
     * @brief Devuelve el porcentaje de inicialización del módulo TTS
     */
    short getTTSInitPercent() const noexcept override;

    /**
     * @brief Obtiene el número de modelos TTS disponibles
     */
    short getAvailableNumTTSModels() const noexcept override;

    /**
     * @brief Obtiene el número de modelos TTS cargados
     */
    short getLoadedNumTTSModels() const noexcept override;
    
    // Añadir aquí métodos de IAppControl...

private:

    /************ Variables ********************************************************/

    // Módulos
    NetMgr      net_;                           // Gestor de sockets de red
    UiMgr       gui_;                           // Gestor de ventanas para la interfaz gráfica
    SoundMgr    snd_;                           // Gestor de audio
    TTSMgr      tts_;                           // Gestor módulo TTS
    LogMgr      log_;

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
