#include "dds/FastDDS.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// Métodos independientes al encapsulado ------------------------------------------------

FastDDS::FastDDS() :
    pimpl_(std::make_unique<Impl>()),
    DOMAIN_ID_(0),
    pqos_name_("DDSMonitorParticipant")
{

}

void FastDDS::loadConfig(void* config) {

    if (!config) 
        return;
        
    // Se considera que la configuración se pasa como json    
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    /* Añadir aquí configuraciones del json: */
    jsonMgr.get_or_set(cfg, "pqos_name", pqos_name_);
    jsonMgr.get_or_set(cfg, "DOMAIN_ID", DOMAIN_ID_);
    
}

#if defined FASTDDS || defined FASTDDS_VERSION

    // Includes de la clase FastDDS
    #include "fastdds_generated/_ALL.hpp"                   // <- TODOS LOS IDLs GENERADOS
    #include <fastdds/dds/domain/DomainParticipantFactory.hpp>
    #include <fastdds/dds/domain/DomainParticipant.hpp>
    #include <fastdds/dds/publisher/Publisher.hpp>
    #include <fastdds/dds/publisher/DataWriter.hpp>
    #include <fastdds/dds/subscriber/Subscriber.hpp>
    #include <fastdds/dds/subscriber/DataReader.hpp>
    #include <fastdds/dds/subscriber/DataReaderListener.hpp>
    #include <fastdds/dds/subscriber/SampleInfo.hpp>

    // Otros includes

    using namespace eprosima::fastdds::dds;

    // Implementación de miembros dependientes de fastdds (pimpl_)
    struct FastDDS::Impl {

        DomainParticipant*      participant;
        Subscriber*             subscriber;
        Publisher*              publisher;
        DomainParticipantQos    pqos;

        /**
         * @brief Constructor de Impl
         * @details Hace el cast de los punteros opacos 
         *  e inicializa las variables miembro de Impl
         */
        Impl();
    };

    // Implementación de métodos PIMPL ------------------------------------------------------
    
    FastDDS::Impl::Impl() :
        participant(nullptr),
        subscriber(nullptr),
        publisher(nullptr)
    {
        
    }



    // General ------------------------------------------------------------------------------

    /* Movido fuera del encapsulado, común en cualquier caso */
    // FastDDS::FastDDS() {...}

    FastDDS::~FastDDS() {

    }


    // Inicialización y ejecución ----------------------------------------------------------------------------
    
    bool FastDDS::init(void* config) {

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

        // Inicialización de DomainParticipantQos, contains all the possible Qos that can be set for a determined participant.
        pimpl_->pqos.name(pqos_name_);

        // Inicialización de participante 
        pimpl_->participant =
            DomainParticipantFactory::get_instance()->create_participant(DOMAIN_ID_, pimpl_->pqos);
        if (!pimpl_->participant) {
            SYS_WARN("FastDDS","Cannot initialize participant");
            return false;
        }


        initialized_ = true;
        return initialized_;    // <- true

    }

    bool FastDDS::isInitialized() const {
        return initialized_;
    }

    /* Movido fuera del encapsulado, común en cualquier caso */
    // Symetrix::loadConfig(void* config) {...}



#else
// ============================================================
//  (Stubs)
// ============================================================

struct FastDDS::Impl {};

// General ------------------------------------------------------------------------------

/* Movido fuera del encapsulado, común en cualquier caso */
// FastDDS::FastDDS() {...}

FastDDS::~FastDDS() { }

// Inicialización y ejecución ----------------------------------------------------------------------------
bool FastDDS::init(void* config) {
    SYS_WARN("FastDDS", "FastDDS library not implemented.");
    loadConfig(config); 
    return false; 
}
bool FastDDS::isInitialized() const    { return false; }

/* Movido fuera del encapsulado, común en cualquier caso */
// Symetrix::loadConfig(void* config) {...}

#endif