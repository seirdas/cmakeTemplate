#include "dds/FastDDS.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// Métodos independientes al encapsulado ------------------------------------------------

FastDDS::FastDDS() :
    pimpl_(std::make_unique<Impl>()),
    initialized_(false),
    enabled_(false),
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
    jsonMgr.get_or_set(cfg, "enable", enabled_);
    
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
    #include <fastdds/dds/topic/TypeSupport.hpp>
    #include <fastdds/dds/core/ReturnCode.hpp>

    // Otros includes

    using namespace eprosima::fastdds::dds;

    /**
     * @brief Listener del DataReader del topic de prueba FastDDSTest.
     * @details FastDDS llama a on_data_available() cuando hay muestras nuevas
     *  esperando a ser leídas en el reader.
     */
    class FastDDSTestListener : public DataReaderListener {
    public:
        void on_data_available(DataReader* reader) override {
            FastDDSTest sample;
            SampleInfo  info;

            // Vaciar la cola: puede haber varias muestras acumuladas
            while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
                if (info.valid_data) {
                    SYS_INFO("FastDDS", "Recibido: id=" + std::to_string(sample.id())
                                         + " message=" + sample.message());
                }
            }
        }
    };

    // Implementación de miembros dependientes de fastdds (pimpl_)
    struct FastDDS::Impl {

        /*
         * DomainParticipant (Crea la conexión al dominio de red)
         * ├── Publisher (Lado que envía)
         * │     └── DataWriter (Escribe datos en un Topic específico)
         * │     └── DataWriter (Otro topic)
         * │     └── DataWriter ...
         * │
         * └── Subscriber (Lado que recibe)
         *       └── DataReader (Lee datos de un Topic específico)
         *       └── DataReader (Otro topic)
         *       └── DataReader ...
         */

        bool                    initialized_;        ///< Bandera que indica inicialización exitosa (pimpl)

        DomainParticipant*      participant_;        ///< Nodo principal de la red en un dominio determinado
        Publisher*              publisher_;          ///< Gestor del lado emisor
        Subscriber*             subscriber_;         ///< Receptor de un topic específico
        DomainParticipantQos    pqos_;


        struct TopicEntry {
            Topic*                                  topic_    = nullptr;
            DataWriter*                             writer_   = nullptr;
            DataReader*                             reader_   = nullptr;
            std::unique_ptr<DataReaderListener>      listener_;  ///< Dueño del listener (FastDDS solo guarda el puntero crudo)
        };

        std::unordered_map<std::string, TopicEntry> topics_;    ///< Lista de tópicos con su reader/listener asociado


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

    FastDDS::FastDDS(FastDDS&&) noexcept = default;
    FastDDS& FastDDS::operator=(FastDDS&&) noexcept = default;


    // Inicialización y ejecución ----------------------------------------------------------------------------
    
    bool FastDDS::init(void* config) {

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
            
        // No hacer nada si no se ha activado
        if (!enabled_) {
            SYS_WARN("FastDDS","FastDDS disabled by config.");
            return false;
        }

        // Inicialización de participante (incluye DomainParticipantQos con tipos de QOS)
        SYS_INFO("FastDDS","Initializing participant (with QOS types)...");
        pimpl_->pqos_.name(pqos_name_);
        pimpl_->participant_ =
            DomainParticipantFactory::get_instance()->create_participant(DOMAIN_ID_, pimpl_->pqos_);
        if (!pimpl_->participant_) {
            SYS_WARN("FastDDS","Cannot initialize participant");
            return false;
        }

        // Crear publisher
        pimpl_->publisher_ = pimpl_->participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!pimpl_->publisher_)
            SYS_WARN("FastDDS","Cannot initialize participant publisher.");

        // Crear subscriber
        pimpl_->subscriber_ = pimpl_->participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!pimpl_->subscriber_)
            SYS_WARN("FastDDS","Cannot initialize participant subscriber.");

        // Comprobar ambos
        if (!pimpl_->subscriber_ && !pimpl_->publisher_) {
            SYS_WARN("FastDDS","Cannot initialize FastDDS server: subscriber and publisher not initialized.");
            return false;
        }


        // Con participant, publisher y subscriber, cargar los tópicos de la config
        if (config)
            loadConfigTopics(config);



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

    void FastDDS::loadConfigTopics(void* config) {

        // Se considera que la configuración se pasa como json    
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();

        // Leer los topics del json en un vector
        std::vector<json*> config_topics = jsonMgr.getArrayElements(cfg, "topics");
        std::string name        = "";
        std::string type_name   = "";
        bool publish            = false;
        bool subscribe          = false;

        for (json* const cfg_node : config_topics) {

            jsonMgr.get_or_set(cfg_node, "name", name);
            jsonMgr.get_or_set(cfg_node, "type", type_name);

            if (name.empty() || type_name.empty()) {
                SYS_WARN("FastDDS", "Entrada de topic inválida en config (falta name/type).");
                continue;
            }
            if (pimpl_->topics_.count(name)) {
                SYS_WARN("FastDDS", "Topic duplicado en config: " + name);
                continue;
            }
            
            /* De alguna forma, crear los topics aquí, con su reader/writer */
        }

    }

    void FastDDS::close() {

        // No hacer nada si el módulo no está activo/inicializado
        if (!enabled_ || !initialized_) return;

        SYS_INFO("FastDDS","Closing domain members...");
        pimpl_->close();

        initialized_ = false;
    }


    // Ejecución ----------------------------------------------------------------------------

    void FastDDS::test() {

        // Crear el topic de prueba si no existe aún (idempotente gracias al check de duplicados)
        if (!pimpl_->topics_.count("FastDDSTestTopic"))
            createTopic("FastDDSTestTopic", true, true);

        auto it = pimpl_->topics_.find("FastDDSTestTopic");
        if (it == pimpl_->topics_.end() || !it->second.writer_) {
            SYS_WARN("FastDDS", "Test data send error: no writer");
            return;
        }

        SYS_INFO("FastDDS","Sending test DDS data...");

        static unsigned int count = 0;
        FastDDSTest msg;
        msg.id(count);
        msg.message("Hola desde FastDDS (C++ API)!");

        if (it->second.writer_->write(&msg) == RETCODE_OK)
            SYS_INFO("FastDDS","Test data sent successfully");
        else
            SYS_WARN("FastDDS","Test data send error");

        count++;
    }


// Topics -------------------------------------------------------------------------------

    bool FastDDS::createTopic(std::string const& typeName, bool withPublish, bool withSubscriber) {

        // #TODO: una vez creado el listener/reader real de un topic de comms/tones,
        // su callback debe mapear los campos del tipo generado por IDL a
        // CommsPacket/TonePacket (mismos structs que usa net/) y llamar a
        // comms_core_->Dispatch(...)/tones_core_->Dispatch(...).
        // Ver el boceto completo en NetMgr::t_dispatcher() (src/net/NetMgr.cpp).

        if (!pimpl_->participant_) {
            SYS_WARN("FastDDS", "Cannot create topic: participant not initialized.");
            return false;
        }

        if (pimpl_->topics_.count(typeName)) {
            SYS_WARN("FastDDS", "Topic duplicado: " + typeName);
            return false;
        }

        // Registrar el tipo FastDDSTest en el participant (hardcodeado de momento, es de prueba)
        TypeSupport type(new FastDDSTestPubSubType());
        type.register_type(pimpl_->participant_);

        // Crear el topic en sí (uno por entrada, aunque el tipo se repita)
        Impl::TopicEntry entry;
        entry.topic_ = pimpl_->participant_->create_topic(
            typeName, type.get_type_name(), TOPIC_QOS_DEFAULT);
        if (!entry.topic_) {
            SYS_WARN("FastDDS", "No se pudo crear topic: " + typeName);
            return false;
        }

        // Writer si hay publisher y la config lo pide
        if (withPublish && pimpl_->publisher_) {
            entry.writer_ = pimpl_->publisher_->create_datawriter(
                entry.topic_, DATAWRITER_QOS_DEFAULT);
            if (!entry.writer_)
                SYS_WARN("FastDDS", "No se pudo crear writer para: " + typeName);
        }

        // Reader si hay subscriber y la config lo pide
        if (withSubscriber && pimpl_->subscriber_) {
            entry.listener_ = std::make_unique<FastDDSTestListener>();
            entry.reader_ = pimpl_->subscriber_->create_datareader(
                entry.topic_, DATAREADER_QOS_DEFAULT, entry.listener_.get());
            if (!entry.reader_)
                SYS_WARN("FastDDS", "No se pudo crear reader para: " + typeName);
        }

        pimpl_->topics_[typeName] = std::move(entry);

        SYS_INFO("FastDDS", "Topic creado: " + typeName + " (tipo " + type.get_type_name() + ")");
        return true;
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
    SYS_WARN("FastDDS", "FastDDS library not included");
    loadConfig(config); 
    return false; 
}
bool FastDDS::isInitialized() const    { return false; }

/* Movido fuera del encapsulado, común en cualquier caso */
// Symetrix::loadConfig(void* config) {...}

void FastDDS::close()                   { return; }
void FastDDS::test()                    { return; }

#endif