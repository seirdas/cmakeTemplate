#include "logic/CommsCore.hpp"
#include "logic/Persona.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación



// General ------------------------------------------------------------------------------

CommsCore::CommsCore(IAppControl* ctrl_) {

}

CommsCore::~CommsCore() {

}


// Inicialización -----------------------------------------------------------------------

bool CommsCore::init(void* config) {
    
}


// Ejecución ----------------------------------------------------------------------------

bool CommsCore::Ejecutar(std::vector<char> data) {
    // Validar que no venga vacío
    if (data.empty()) 
        return false;

    // Comparar el hash del nuevo paquete con el procesado justo antes
    std::string_view sv(data.data(), data.size());
    size_t hash_actual = std::hash<std::string_view>{}(sv);
    if (hash_actual == last_packet_hash_)
        return false;
    last_packet_hash_ = hash_actual;


    // vvvvvvvv La lógica de comms aquí vvvvvvvvv
    
    /* #TODO */

    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


    return true;
}
