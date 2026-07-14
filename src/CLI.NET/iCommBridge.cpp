#include "CLI.NET/iCommBridge.hpp"
#include "CLI.NET/iCommWrapper.hpp"
#include "tts/TTSMgr.hpp"
#include <vcclr.h>

iCommBridge::iCommBridge(TTSMgr* parent) {
    // Crea el ref class y lo protege en modo gestionado
    gcroot<iCommWrapper^>* handle = new gcroot<iCommWrapper^>(gcnew iCommWrapper(parent));
    m_managedWrapper = static_cast<void*>(handle);
}

iCommBridge::~iCommBridge() {
    if (m_managedWrapper) {
        auto handle = static_cast<gcroot<iCommWrapper^>*>(m_managedWrapper);
        delete handle; // Liberamos la referencia para el Garbage Collector
        m_managedWrapper = nullptr;
    }
}

bool iCommBridge::init() {
    auto handle = static_cast<gcroot<iCommWrapper^>*>(m_managedWrapper);
    return (*handle)->init();
}

bool iCommBridge::close() {
    auto handle = static_cast<gcroot<iCommWrapper^>*>(m_managedWrapper);
    return (*handle)->close();
}