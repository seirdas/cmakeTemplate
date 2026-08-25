#include "logic/tts/TTSDispatcher.hpp"
#include "files/JsonMgr.hpp"
#include "system/SystemMgr.hpp"
#include "CLI.NET/iCommBridge.hpp"  // Puente a clase administrada (CLI.NET) iCommMgr

#include <thread>


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

TTSDispatcher::TTSDispatcher(IAppControl* ctrl) :
    pimpl_(std::make_unique<Impl>(this)),
    initialized_(false),
    ctrl_(ctrl),
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
    
    // Hilo consumidor de paquetes TTS (del iComm)
    dataConsumer_thread_ = std::thread(&TTSDispatcher::t_data_consumer, this);
    
    // Marcar el módulo internamente como inicializado y corriendo
    initialized_    = true;
    running_        = true;

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
    
    // Espera a que se cierre el hilo consumidor
    if (dataConsumer_thread_.joinable()) {
        SYS_INFO("TTSMgr","Waiting for consumer thread...");
        dataConsumer_thread_.join();
    }

    initialized_ = false;
    return !initialized_;   // <- true
}

// Ejecución ----------------------------------------------------------------------------

    bool Dispatch() {

        return false;
    }


// Hilos --------------------------------------------------------------------------------

void TTSDispatcher::t_data_consumer() {

    while (running_) {

        // Salir si el programa se está cerrando (antes de bloqueo)
        if (!running_)
            break;

        // Forzar la espera hasta que sea notificado de un paquete nuevo
        std::unique_lock<std::mutex> lock(queue_mtx_);
        queue_cv_.wait(lock, [this] {
            return !running_ || !queue_.empty();
        });

        // Salir si el programa se está cerrando (después de bloqueo)
        if (!running_)
            break;

        // Va consumiendo la cola de datos pasándoselo a los ttsPlayers
        /* #TODO */

        // Lógica de ejemplo (borrar al implementar la de verdad)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }
}
