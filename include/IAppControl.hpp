#pragma once

#include <string>               // Maneja cadenas de texto

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
  */
class IAppControl {

public:

    /**
     * @brief Destructor virtual para asegurar la correcta destrucción de objetos derivados.
     */
    virtual ~IAppControl() = default;

    /**
     * @brief Obtiene la versión de la aplicación.
     */
    virtual std::string getVersion() const noexcept = 0;
    
    /**
     * @brief Método ejemplo para pedir información a uno de los miembros. 
     * @return El puerto en el que el receptor UDP está escuchando.
     */
    virtual int get_SocketPort() const noexcept = 0;

};