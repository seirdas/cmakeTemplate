#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"

// Declaración implícita
class IAppController;
class VoIPRec;
class VoIPPlay;

// añadido temporalmente, después se pasa al cpp
#include "links/VoIPPlay.hpp"
#include "links/VoIPRec.hpp"


/**
 * @class VoIPMgr
 * @brief Clase gestora de Voiprec/Voipplay
 */
class VoIPMgr {

public:

// General ------------------------------------------------------------------------------

    VoIPMgr() {

    }

    ~VoIPMgr() {

    }

    bool init(void* config) {

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else
            SYS_WARN("VoIPMgr","Cannot load config. Using default values.");


        // #TODO
    }

    void clear() {
        voiprecs_.clear();
        voipplays_.clear();
    }

    void loadConfig(void* config) {

        if (!config) 
            return;

        SYS_INFO("VoIPMgr","Reading config node...");
            
        // Se considera que la configuración se pasa como json    
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        /* Aquí cargar la config, iniciar los voiprec/plays, etc... */
        // #TODO
    }


// VoIPRec ------------------------------------------------------------------------------


    bool add_voiprec() {

        // comprobaciones

        // inicializar voiprec

        // añadir al vector

    }

    bool remove_voiprec(std::string name) { 

        // buscar voiprec (nombre en map)

        // parar voiprec

        // eliminar del vector
    }


// VoIPPlay -----------------------------------------------------------------------------
    
    bool add_voipplay() {

        // comprobaciones

        // inicializar voipplay

        // añadir al vector
    }

    bool remove_voipplay(std::string name) { 

        // buscar voipplay (nombre en map)

        // parar voipplay

        // eliminar del vector
    }



private:


/************ Variables ********************************************************/

    std::unordered_map<std::string, std::unique_ptr<VoIPRec>>   voiprecs_;
    std::unordered_map<std::string, std::unique_ptr<VoIPPlay>>  voipplays_;

};
