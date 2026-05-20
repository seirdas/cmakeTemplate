#pragma once

/**
 * @class CycloneDDS
 * @brief Gestor de comunicaciones basado en el protocolo Data Distribution Service (DDS) mediante CycloneDDS.
 * 
 * Esta clase abstrae la lógica de publicación y suscripción de tópicos utilizando el middleware
 * CycloneDDS. Permite el intercambio de datos a alta velocidad y en tiempo real con otros nodos
 * de la red de forma desacoplada.
 */
class CycloneDDS {

public:

// General ------------------------------------------------------------------------------
    
    /**
     * @brief Constructor estándar 
     */
    CycloneDDS();

    /**
     * @brief Destructor estándar 
     */
    ~CycloneDDS();

    /**
     * @brief Ejemplo de uso de CycloneDDS 
     */
    void test();


// Dejo aquí el código para la implementación del publisher y subscriber de cyclone -----

    /**
     * @brief Publica datos del idl en un intervalo de tiempo
     * @warning BLOQUEANTE 
     */
    void run_publisher();

    /**
     * @brief Recibe datos del idl 
     * @warning BLOQUEANTE 
     */
    void run_subscriber();


private:

    /* Variables */

};


/* Prueba cyclonedds en main */

/*
// Leer el argumento de la consola para decidir qué rol tomar
std::string mode = "pub"; // Por defecto es publicador

if (argc > 1)
    mode = argv[1];

if (mode == "sub")
    run_subscriber();       // <- bloqueante
else if (mode == "pub")
    run_publisher();        // <- bloqueante

*/

