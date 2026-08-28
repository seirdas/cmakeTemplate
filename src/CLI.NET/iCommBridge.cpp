#include "CLI.NET/iCommBridge.hpp"
#include "CLI.NET/iCommMgr.hpp"
#include "dispatchers/TTSDispatcher.hpp"
#include <vcclr.h>


// General ------------------------------------------------------------------------------

iCommBridge::iCommBridge() :
    initialized_(false)
{
    // Crea el ref class y lo protege en modo gestionado    
    gcroot<iCommMgr^>* handle = new gcroot<iCommMgr^>(gcnew iCommMgr());
    managedWrapper_ = static_cast<void*>(handle);
}

iCommBridge::~iCommBridge() {
    if (managedWrapper_) {
        gcroot<iCommMgr^>* handle = static_cast<gcroot<iCommMgr^>*>(managedWrapper_);
        delete handle; // Liberamos la referencia para el Garbage Collector
        managedWrapper_ = nullptr;
    }
}

// Inicialización y ejecución -----------------------------------------------------------

bool iCommBridge::init(void* config) {
    if (initialized_)
        return false;

    auto handle = static_cast<gcroot<iCommMgr^>*>(managedWrapper_);
    initialized_ = (*handle)->init();
    return initialized_;
}

bool iCommBridge::isInitialized() const {
    return initialized_;
}

bool iCommBridge::close() {
    if (!initialized_)
        return false;

    gcroot<iCommMgr^>* handle = static_cast<gcroot<iCommMgr^>*>(managedWrapper_);
    initialized_ = !(*handle)->close();
    return !initialized_;
}
