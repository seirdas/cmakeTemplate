#pragma once

// Declaración anticipada para evitar incluir TTSMgr.hpp aquí
class TTSMgr;

/**
 * @class iCommBridge
 * @brief Clase puente (bridge) para la comunicación con iComm (.NET).
 * 
 * Esta clase actúa como interfaz nativa para gestionar la interacción con la librería 
 * administrada (managed) de iComm. Utiliza un patrón PIMPL para encapsular el 
 * manejo de objetos gestionados mediante @c gcroot, permitiendo que la lógica 
 * nativa del proyecto pueda invocar funciones .NET y recibir callbacks 
 * de manera transparente.
 */
class iCommBridge {
public:

// General ------------------------------------------------------------------------------

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

// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa el cliente iComm.
     *  - Lee el archivo de configuración de iComm (iCommConfigFile)
     *  - Obtiene los identificadores de ATIS/ATC
     *  - Suscribe eventos a funciones que ejecutará TTSMgr
     * @return @c true Si la inicialización ha sido correcta, @c false en caso contrario
     */
    bool init();

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
     * @brief Para la conexión y las suscripciones del iComm
     *  y borra el puntero a la instancia iCommMgr
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close();

private:

// Inicialización
    bool    initialized_;   ///< Bandera para indicar inicialización exitosa

// Miembro puente
    void*   managedWrapper_; ///< Puntero opaco que esconderá el gcroot interno

};

#if not defined _MSC_VER
// ============================================================
//  (Stubs para no MSVC)
//  NOTA:
//      Los Stubs se ponen aquí porque si el compilador no es MSVC, 
//      los archivos fuente cpp no se incluyen en la compilación
// ============================================================

    #include "system/SystemMgr.hpp"

// General ------------------------------------------------------------------------------
    inline iCommBridge::iCommBridge(TTSMgr*) : initialized_(true)   {}
    inline iCommBridge::~iCommBridge()              {}

// Ejecución ----------------------------------------------------------------------------
    inline bool iCommBridge::init()     { 
        SYS_WARN("iCommBridge","iCommWrapper only compatible with MSVC compiler (Windows)");
        return false; 
    }
    inline bool iCommBridge::isInitialized() const  { return false; }
    inline bool iCommBridge::close()    { return false; }
#endif
