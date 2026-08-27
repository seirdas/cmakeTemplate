#include "app/IModule.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

IModule::IModule() :
    initialized_(false),
    threads_running_(false)
{

}

IModule::~IModule() = default;


// Inicialización -----------------------------------------------------------------------

bool IModule::init(void* config) {
    // Si ya está inicializado no hacer nada
    if (initialized_)
        return true;

    // Guardar la configuración del módulo
    config_ = config;

    // Cargar la configuración (si existe)
    if (config_)
        loadConfig(config_);

    // Método que debe ser sobreescrito por el módulo
    if(!onInit())
        return false;

    threads_running_    = true;
    initialized_        = true;
    return true;
}

bool IModule::isInitialized() const {
    return initialized_;
}


// Configuración ------------------------------------------------------------------------

bool IModule::setController(IAppControl* controller) {
    ctrl_ = controller;
    return static_cast<bool>(ctrl_);
}


// Cierre -------------------------------------------------------------------------------

bool IModule::close() {
    if (!initialized_)
        return true;

    if(!onClose())
        return false;

    // Parar flag para hilos
    threads_running_ = false;

    // Guardar estado de no inicializado
    initialized_ = false;

    // Devolver cierre correcto
    return true;
}


// Inicialización y cierre específico de módulo -----------------------------------------

bool IModule::onInit() {
    // Si la derivada no sobrescribe 'onInit', se ejecuta esto:
    SYS_WARN("IModule","Using default initialization (without override)");
    return false;
}

bool IModule::onClose() {
    // Si la derivada no sobrescribe 'onInit', se ejecuta esto:
    SYS_WARN("IModule","Using default close method (without override)");
    return false;
}
