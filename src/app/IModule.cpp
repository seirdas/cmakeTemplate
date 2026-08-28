#include "app/IModule.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

IModule::IModule() :
    initialized_(false),
    threads_running_(false)
{

}

IModule::~IModule() = default;


// Inicialización y cierre --------------------------------------------------------------

bool IModule::init(void* config) {
    // Si ya está inicializado no hacer nada
    if (initialized_) {
        SYS_INFO("IModule","Already initialized");
        return false;
    }

    // Guardar la configuración del módulo
    config_ = config;

    // Cargar la configuración (si existe)
    if (config_)
        loadConfig(config_);

    threads_running_    = true;
    initialized_        = true;
    return true;
}

bool IModule::close() {
    if (!initialized_)
        return false;

    // Parar flag para hilos
    threads_running_ = false;

    // Guardar estado de no inicializado
    initialized_ = false;

    // Devolver cierre correcto
    return true;
}


// Configuración ------------------------------------------------------------------------

/* ... */


// Parámetros del módulo ----------------------------------------------------------------

bool IModule::isInitialized() const {
    return initialized_;
}


// Configuración ------------------------------------------------------------------------

bool IModule::setController(IAppControl* controller) {
    ctrl_ = controller;
    return static_cast<bool>(ctrl_);
}
