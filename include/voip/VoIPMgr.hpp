#pragma once

#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

// Declaración implícita
class IAppController;
class VoIPRec;
class VoIPPlay;

// añadido temporalmente, después se pasa al cpp
#include "voip/VoIPPlay.hpp"
#include "voip/VoIPRec.hpp"


/**
 * @class VoIPMgr
 * @brief Clase gestora de Voiprec/Voipplay
 */
class VoIPMgr {

public:

// General ------------------------------------------------------------------------------

    VoIPMgr() :
        createVoiprec_(nullptr)
    {

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

        // hago un vector que apunte al array de los nodos json dentro del nodo principal
        std::vector<json*> config_voiprec = jsonMgr.getArrayElements(cfg, "voiprec");

        // Datos para crear Voiprec (WIP)
        std::string name;
        std::string socketName;
        std::string audioSourceName;
        for (json* const cfg_node : config_voiprec) {
            jsonMgr.get_or_set(cfg_node, "name", name);

            if(!name.empty())
                createVoiprec_(name);   // Función inyectada para usar snd, net
        }

        SYS_INFO("VoIPMgr","Config node read OK");

    }


// VoIPRec ------------------------------------------------------------------------------

    bool add_voiprec(const std::string& name, std::function<void(const std::vector<char>&)> send_cb) {
        // Comprobaciones: Si ya existe, no crearlo
        if (voiprecs_.find(name) != voiprecs_.end()) return false;
        
        // Añadir el nuevo VoIPRec al vector (faltaría inicializarlo)
        voiprecs_[name] = std::make_unique<VoIPRec>(name, std::move(send_cb));
        return true;
    }

    bool remove_voiprec(std::string name) { 

        // buscar voiprec (nombre en map)
        auto it = voiprecs_.find(name);
        if (it == voiprecs_.end())
            return false;

        // parar voiprec
        if (it->second)
            it->second->stop(); 

        // eliminar del vector
        voiprecs_.erase(it);
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

    std::function<void(std::string name)>  createVoiprec_;     ///< Función inyectada para usar snd, net

};
 