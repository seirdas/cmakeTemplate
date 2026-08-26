#include "dispatchers/TTSDispatcher.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "CLI.NET/iCommBridge.hpp"  // Puente a clase administrada (CLI.NET) iCommMgr



// Implementación de puente para clase administrada (iComm)
struct TTSDispatcher::Impl {
    iCommBridge commBridge;
    
    /**
     * @brief Constructor
     * @param parent Esta misma clase, para que icomm ejecute
     *  las funciones públicas de esta clase (Dispatch)
     */
    Impl(TTSDispatcher* parent) 
        : commBridge(parent) {}
};


// General ------------------------------------------------------------------------------

TTSDispatcher::TTSDispatcher() :
    pimpl_(std::make_unique<Impl>(this)),
    initialized_(false),
    ctrl_(nullptr),
    last_packet_hash_(0)
{

}

TTSDispatcher::~TTSDispatcher() {
    close();
}


// Inicialización -----------------------------------------------------------------------

bool TTSDispatcher::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else  // Puede llegar aquí cuando se hace reload()
        SYS_WARN("TTSDispatcher","Cannot load config. Using default values.");
    

    // Inicialización de iComm (CLI.NET)
    SYS_INFO("TTSMgr","Starting iComm (.NET) client...");
    if(!pimpl_->commBridge.init())
        SYS_WARN("TTSMgr","iComm FAIL");
    else SYS_INFO("TTSMgr","iComm OK");
    
    // Marcar el módulo internamente como inicializado y corriendo
    initialized_    = true;

    return initialized_;    //<- true
}

bool TTSDispatcher::isInitialized() const {
    return initialized_;
}

void TTSDispatcher::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // #TODO

}

bool TTSDispatcher::close() {

    // Comprobar si el módulo ya estaba cerrado
    if (!initialized_) return true;

    // Cierra el cliente de iComm
    if (pimpl_->commBridge.isInitialized()) {
        SYS_INFO("TTSMgr","Closing iComm (.NET) client...");
        if(!pimpl_->commBridge.close())
            SYS_WARN("TTSMgr","Closing iComm FAIL");
    }

    initialized_ = false;
    return !initialized_;   // <- true
}

// Ejecución ----------------------------------------------------------------------------

bool Dispatch() {

    return false;
}
