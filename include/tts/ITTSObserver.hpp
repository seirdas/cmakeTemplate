// TTSObserver.hpp
#include "datatypes/TTSDataTypes.hpp"

class ITTSObserver {
public:

    /**
     * @brief Destructor de interfaz observer
     * @details Destructor virtual asegura que se ejecute primero 
     *  el destructor de la clase derivada y luego ésta
     * @details ``= default`` para implementación de destructor por defecto
     */
    virtual ~ITTSObserver() = default;

// Métodos que sobreescribirá GuiMgr ----------------------------------------------------
    
    /**
     * @brief Ejecuta acciones en función de los datos recibidos 
     *  por parámetro del TTS
     * @details Lo llama TTSMgr -> lo define GuiMgr lo que va a hacer
     * @param data 
     */
    virtual void onTTSDataChanged(TTSCoreData const& data) = 0;
};
