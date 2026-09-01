#pragma once

#include <functional>

// Declaración anticipada para evitar incluir TTSPacket.hpp aquí
struct TTSPacket;

/**
 * @class iCommBridge
 * @brief Clase puente (bridge) para la comunicación con iComm (.NET).
 * @note No puede heredar de IModule porque se mezcla con clases no administradas
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
     * @brief Constructor
     */
    iCommBridge();

    /**
     * @brief Destructor
     *  Elimina las referencias a memoria de la clase administrada
     */
    ~iCommBridge();


// Inicialización y ejecución -----------------------------------------------------------

    /**
     * @brief Inicializa el cliente iComm.
     *  - Lee el archivo de configuración de iComm (iCommConfigFile)
     *  - Obtiene los identificadores de ATIS/ATC
     *  - Suscribe eventos a funciones que ejecutará TTSMgr
     * @return @c true Si la inicialización ha sido correcta, @c false en caso contrario
     */
    bool init(void* config);

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

    /**
     * @brief Registra el callback al que se le entregará cada TTSPacket recibido.
     * @details Lo conecta AppController, normalmente hacia TTSDispatcher::Dispatch().
     * @param cb Callback a invocar con el paquete recibido.
     */
    void setCallback_onReceive(std::function<void(TTSPacket&)> cb);


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
    inline iCommBridge::iCommBridge() : initialized_(false)   {}
    inline iCommBridge::~iCommBridge()              {}

// Ejecución ----------------------------------------------------------------------------
    inline bool iCommBridge::init(void*)     { 
        SYS_WARN("iCommBridge","iCommMgr only compatible with MSVC compiler (Windows)");
        return initialized_; 
    }
    inline bool iCommBridge::isInitialized() const  { return false; }
    inline bool iCommBridge::close()    { return false; }
    inline void iCommBridge::setCallback_onReceive(std::function<void(TTSPacket&)>) { }
#endif
