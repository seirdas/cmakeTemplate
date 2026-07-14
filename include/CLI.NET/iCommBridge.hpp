#pragma once

// Declaración anticipada para evitar incluir TTSMgr.hpp aquí
class TTSMgr;

/**
 * @class iCommBridge
 * @brief Clase puente (bridge) para la comunicación con componentes .NET.
 * 
 * Esta clase actúa como interfaz nativa para gestionar la interacción con la librería 
 * administrada (managed) de iComm. Utiliza un patrón PIMPL para encapsular el 
 * manejo de objetos gestionados mediante @c gcroot, permitiendo que la lógica 
 * nativa del proyecto pueda invocar funciones .NET y recibir callbacks 
 * de manera transparente.
 */
class iCommBridge {
public:
    /**
     * @brief Constructor.
     * @param parent Puntero a TTSMgr para que el código .NET pueda llamarlo de vuelta
     */
    iCommBridge(TTSMgr* parent);

    /**
     * @brief Destructor
     *  Elimina las referencias a memoria de la clase administrada
     */
    ~iCommBridge();

    /**
     * @brief Inicializa el cliente iComm.
     *  - Lee el archivo de configuración de iComm (iCommConfigFile)
     *  - Obtiene los identificadores de ATIS/ATC
     *  - Suscribe eventos a funciones que ejecutará TTSMgr
     * @return @c true Si la inicialización ha sido correcta, @c false en caso contrario
     */
    bool init();

    /**
     * @brief Para la conexión y las suscripciones del iComm
     *  y borra el puntero a la instancia iCommMgr
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close();

private:

// Miembro puente
    void* m_managedWrapper; ///< Puntero opaco que esconderá el gcroot interno

};