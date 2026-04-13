#pragma once

#include <string>               // Maneja cadenas de texto
#include <vector>

/**
 * @brief Interfaz de intercomunicación entre los miembros de la aplicación.
 *
 * @details
 * Permite que los componentes de la aplicación se comuniquen sin estar acoplados
 * directamente, facilitando la modularidad y el mantenimiento. Cada miembro puede
 * implementar esta interfaz para exponer métodos que otros puedan invocar,
 * sin necesidad de conocer la implementación concreta de cada uno.
 *
 * @note
 * Los constructores de los miembros deben recibir un puntero a esta interfaz
 * para poder registrarse y comunicarse con otros miembros.
 *
 * @note
 * Esta clase actúa como plantilla de funciones (interfaz) que pueden llamarse
 * desde los miembros, y deben sobrescribirse (`override`) en la clase principal
 * **AppController** que implementa la interfaz.
 *
 * @example MyClass.cpp
 * @include IAppControl.hpp
 *
 * ```cpp
 * // MyClass.hpp
 * #include "IAppControl.hpp"
 *
 * class MyClass {
 * public:
 *     explicit MyClass(IAppControl* controller) : m\_controller(controller) {}
 *
 * private:
 *     IAppControl* m\_controller;
 * };
 *
 * // Uso:
 * AppController appCtrl;          // Clase que implementa IAppControl
 * MyClass       obj(&appCtrl);    // Se pasa el puntero a la interfaz
 * ```
 *
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

    /**
     * @brief Obtiene el número de modelos TTS disponibles
     */
    virtual short getAvailableNumTTSModels() const noexcept = 0;

    /**
     * @brief Obtiene el número de modelos TTS cargados
     */
    virtual short getLoadedNumTTSModels() const noexcept = 0;

    /**
     * @brief Obtiene el texto procesado por un modelo TTS 
     */
    virtual std::string getTTSProcessingText(std::string modelName) const noexcept = 0;

};
