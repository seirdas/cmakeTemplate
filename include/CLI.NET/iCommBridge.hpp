#pragma once

// Declaración anticipada para evitar incluir TTSMgr.hpp aquí
class TTSMgr;

// Esta clase es 100% nativa de cara al exterior.
class iCommBridge {
public:
    // Le pasamos el puntero a TTSMgr para que el código .NET pueda llamarlo de vuelta
    iCommBridge(TTSMgr* parent);

    ~iCommBridge();

    bool init();

private:
    // Puntero opaco que esconderá el gcroot interno
    void* m_managedWrapper; 
};