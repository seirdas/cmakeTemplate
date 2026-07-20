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

        /*
         * DomainParticipant (Crea la conexión al dominio de red)
         * ├── Publisher (Lado que envía)
         * │     └── DataWriter (Escribe datos en un Topic específico)
         * │
         * └── Subscriber (Lado que recibe)
         *         └── DataReader (Lee datos de un Topic específico)
        */

        bool                    initialized_;        ///< Bandera que indica inicialización exitosa (pimpl)

        DomainParticipant*      participant_;        ///< Nodo principal de la red en un dominio determinado
        Publisher*              publisher_;          ///< Gestor del lado emisor
        Subscriber*             subscriber_;         ///< Receptor de un topic específico
        DomainParticipantQos    pqos_;


        /**
         * @brief Constructor de Impl
         * @details Hace el cast de los punteros opacos 
         *  e inicializa las variables miembro de Impl
         */
        Impl();

        /**
         * @brief Destructor de Impl
         * @details Libera la memoria creada y los recursos asociados
         *  a las instancias de las clases de FastDDS
         */
        ~Impl();

        /**
         * @brief Método auxiliar para liberar recursos
         */
        void close();
    };

    // Implementación de métodos PIMPL ------------------------------------------------------
    
    FastDDS::Impl::Impl() :
        participant_(nullptr),
        subscriber_(nullptr),
        publisher_(nullptr)
    {
        initialized_ = true;
    }

    FastDDS::Impl::~Impl() {
        close();
    }

    void FastDDS::Impl::close() {
        if (!initialized_) return;

        if (participant_) {

            // Borra el publisher del participant (si aplica)
            if (publisher_) {
                SYS_INFO("FastDDS","Deleting publisher...");
                participant_->delete_publisher(publisher_);
            }

            // Borra el subscriber del participant (si aplica)
            if (subscriber_) {
                SYS_INFO("FastDDS","Deleting subscriber...");
                participant_->delete_subscriber(subscriber_);
            }

            // Borra el participante
            SYS_INFO("FastDDS","Deleting participant...");
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }

        initialized_ = false;
    }



    // General ------------------------------------------------------------------------------

    /* Movido fuera del encapsulado, común en cualquier caso */
    // FastDDS::FastDDS() {...}

    FastDDS::~FastDDS() {
        close();
    }


    // Inicialización y ejecución ----------------------------------------------------------------------------
    
    bool FastDDS::init(void* config) {

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);

        // Inicialización de DomainParticipantQos, contains all the possible Qos that can be set for a determined participant.
        pimpl_->pqos_.name(pqos_name_);

        // Inicialización de participante 
        pimpl_->participant_ =
            DomainParticipantFactory::get_instance()->create_participant(DOMAIN_ID_, pimpl_->pqos_);
        if (!pimpl_->participant_) {
            SYS_WARN("FastDDS","Cannot initialize participant");
            return false;
        }

        /* Demás implementación de fastdds...*/
        // #TODO


        initialized_ = true;
        return initialized_;    // <- true

    }

    bool FastDDS::isInitialized() const {
        return initialized_;
    }

    /* Movido fuera del encapsulado, común en cualquier caso */
    // Symetrix::loadConfig(void* config) {...}

    void FastDDS::close() {

        if (!initialized_) return;

        SYS_INFO("FastDDS","Closing domain members...");
        pimpl_->close();

        initialized_ = false;
    }


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

void FastDDS::close()                   { return; }

#endif