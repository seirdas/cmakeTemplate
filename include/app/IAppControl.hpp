#pragma once

#include <string>               // Maneja cadenas de texto


// Foward declaration
class NetMgr;
class SoundMgr;
class TotalMix;
class Symetrix;


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
 * #include "app/IAppControl.hpp"
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
     * @details Destructor virtual asegura que se ejecute primero 
     *  el destructor de la clase derivada y luego ésta
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


// Módulos --------------------------------------------------------------------------

    /**
     * @brief Devuelve la instancia de gestor de red
     * @return Puntero a gestor de red
     */
    virtual NetMgr* getNetModule() = 0;

    /**
     * @brief Devuelve la instancia de gestor de sonidos
     * @return Puntero a gestor de sonidos
     */
    virtual SoundMgr* getSoundsModule() = 0;

    /**
     * @brief Devuelve la instancia de gestor de Totalmix
     * @return Puntero a gestor de totalmix
     */
    virtual TotalMix* getTotalmixModule() = 0;

    /**
     * @brief Devuelve la instancia de gestor de Symetrix
     * @return Puntero a gestor de symetrix
     */
    virtual Symetrix* getSymetrixModule() = 0;

};
