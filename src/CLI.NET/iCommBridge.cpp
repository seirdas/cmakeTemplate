#include "CLI.NET/iCommBridge.hpp"
#include "CLI.NET/iCommWrapper.hpp"
#include "tts/TTSMgr.hpp"
#include <vcclr.h>

// General ------------------------------------------------------------------------------

iCommBridge::iCommBridge(TTSMgr* parent) :
    initialized_(true)
{
    // Crea el ref class y lo protege en modo gestionado
    gcroot<iCommWrapper^>* handle = new gcroot<iCommWrapper^>(gcnew iCommWrapper(parent));
    managedWrapper_ = static_cast<void*>(handle);
}

iCommBridge::~iCommBridge() {
    if (managedWrapper_) {
        auto handle = static_cast<gcroot<iCommWrapper^>*>(managedWrapper_);
        delete handle; // Liberamos la referencia para el Garbage Collector
        managedWrapper_ = nullptr;
    }
}

// Inicialización y ejecución -----------------------------------------------------------

bool iCommBridge::init() {
    auto handle = static_cast<gcroot<iCommWrapper^>*>(managedWrapper_);
    return (*handle)->init();
}

bool iCommBridge::isInitialized() const {
    return initialized_;
}

bool iCommBridge::close() {
    auto handle = static_cast<gcroot<iCommWrapper^>*>(managedWrapper_);
    return (*handle)->close();
}