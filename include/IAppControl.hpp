#pragma once

#include <string>               // Maneja cadenas de texto
#include <vector>

/**
  * @brief Interfaz de intercomunicación entre miembros de la aplicación. 
  *  La clase de la aplicación debe extenderse desde esta interfaz.
  * @details Permite a los miembros de la aplicación comunicarse entre sí sin acoplarse directamente, 
  *  facilitando la modularidad y el mantenimiento.
  *  Cada miembro puede implementar esta interfaz para 
  *  exponer métodos que otros miembros pueden llamar, sin 
  *  necesidad de conocer la implementación concreta de cada miembro.
  * @note Los constructores de los miembros deben recibir un puntero 
  *  a esta interfaz para poder registrarse y comunicarse con otros miembros.
  * @note Esta clase actúa como plantilla de funciones (interfaz) 
  *  que pueden llamarse desde los miembros, y deberían estar definidas como 
  *  "override" en la clase principal AppController que extiende esta interfaz.
  * @example MyClass(IAppControl* controller), con #include "IAppControl.hpp". 
  *  Instanciarlo con un parámetro MyClass inst(AppController). 
  *  Sólo leerá los métodos de IAppControl con las funciones definidas en AppController.
  * @see AppController
  */
class IAppControl {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Destructor virtual para asegurar la correcta destrucción de objetos derivados.
     */
    virtual ~IAppControl() = default;

// Aplicación ---------------------------------------------------------------------------

    /**
     * @brief Obtiene la versión de la aplicación.
     */
    virtual std::string getVersion() const noexcept = 0;
    
    /**
     * @brief Establece el modo de la aplicación.
     * @param newMode true = online, false = offline
     */
    virtual void setOnlineMode(bool newMode) noexcept = 0;

    /**
     * @brief Obtiene el estado Online/Offline
     * @returns true = online, false = offline
     */
    virtual bool isOnlineMode() const noexcept = 0;


// Sockets ------------------------------------------------------------------------------

    /**
     * @brief Agrega un nuevo receptor de datos de red.
     * @return true si el receptor se agregó correctamente, false en caso de error.
     */
    virtual bool addReceiver() const noexcept = 0;

    /**
     * @brief Elimina un receptor de datos de red.
     * @return true si el receptor se eliminó correctamente, false en caso de error.
     */
    virtual bool removeReceiver() const noexcept = 0;


// Audio --------------------------------------------------------------------------------

    /**
     * @brief Devuelve la lista con los dispositivos de entrada disponibles.
     */
    virtual std::vector<std::string> getAvailableInputDevices() noexcept = 0;
    
    /**
     * @brief Devuelve la lista con los dispositivos de reproducción disponibles.
     */
    virtual std::vector<std::string> getAvailablePlaybackDevices() noexcept = 0;

// TTS --------------------------------------------------------------------------------

    /**
     * @brief Devuelve el porcentaje de inicialización del módulo TTS
     */
    virtual short getTTSInitPercent() const noexcept = 0;

};
