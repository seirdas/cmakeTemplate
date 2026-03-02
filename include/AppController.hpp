
#pragma once

#include <string>               // String
#include <thread>               // Hilos
#include <fstream>              // Gestiona archivos

#include <json.hpp>             // Manipula archivos .json

#include "winMgr.h"             // Clase de gestión de ventana UI
#include "NetMgr.hpp"           // Clase para gestionar sockets
#include "SoundMgr.hpp"         // Clase para gestionar audio
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

// IAppControl methods ------------------------------------------------------------
    /**
     * @brief Implementación del método de IAppControl para devolver la versión de la aplicación.
     * @return La versión de la aplicación como una cadena de texto.
     * @note const evita que el método modifique la variable "version_" o cualquiera.
     */
    std::string getVersion() const noexcept override { return version_; }

    /**
     * @brief Implementación del método de IAppControl para devolver el puerto en el que el receptor UDP está escuchando.
     * @return El puerto del receptor UDP. Si el receptor no está en ejecución, devuelve -1.
     * @note const evita que el método modifique cualquier variable miembro de la clase.
     */
    int get_SocketPort() const noexcept override { return 1; }

    // Añadir aquí métodos de IAppControl...

private:

    /************ Variables ********************************************************/

    NetMgr      net_;                               // Gestor de sockets de red
    WinMgr      ui_;                                // Gestor de ventanas para la interfaz gráfica
    SoundMgr    snd_;                               // Gestor de audio

    std::string version_ = std::to_string(VERSION); // Versión de la aplicación
};
