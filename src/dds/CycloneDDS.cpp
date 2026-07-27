#include "dds/CycloneDDS.hpp"
#include "system/SystemMgr.hpp"


#if defined CYCLONEDDSCXX || defined CYCLONEDDSCXX_VERSION


    /* ---- ES NORMAL QUE SALGAN ERRORES: */
    // Se necesita al menos compilar una vez para CYCLONEDDS se instale 
    // en el proyecto y no aparezcan los errores en los componentes de cyclonedds
    // Si aun así salen errores, borrar la carpeta '_build' y reconfigurar de nuevo

    // Include de los archivos .idl en formato .hpp generados por cyclone
    #include "cyclone_generated/_ALL.hpp"
    #include "dds/dds.hpp"
    #include <thread>


    // Implementación de miembros dependientes de cyclonedds (pimpl_)
    struct CycloneDDS::Impl {

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

        bool initialized_;       ///< Bandera para indicar inicialización exitosa

        dds::domain::DomainParticipant  participant_;
        dds::pub::Publisher             publisher_;
        dds::sub::Subscriber            subscriber_;
        dds::topic::Topic<HelloWorld>   topic_;


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

    CycloneDDS::Impl::Impl() :
        initialized_(false),
        participant_(dds::core::null),
        publisher_(dds::core::null),
        subscriber_(dds::core::null),
        topic_(dds::core::null) 
    {

    }

    CycloneDDS::Impl::~Impl() {
        close();
    }

    void CycloneDDS::Impl::close() {
        initialized_ = false;
    }




    // General ------------------------------------------------------------------------------

    CycloneDDS::CycloneDDS() :
        pimpl_(std::make_unique<Impl>()),
        initialized_(false),
        enabled_(true),
        DOMAIN_ID_(0),
        sub_running_(false)
    {

    }

    CycloneDDS::~CycloneDDS() {
        close();
    }


    // Inicialización y ejecución ----------------------------------------------------------------------------

    bool CycloneDDS::init(void* config) {
        DOMAIN_ID_ = 0;

        try {
            // Sobrescribimos el estado nulo asignando los objetos recién creados
            pimpl_->participant_ = dds::domain::DomainParticipant(DOMAIN_ID_);
            pimpl_->publisher_   = dds::pub::Publisher(pimpl_->participant_);
            pimpl_->subscriber_  = dds::sub::Subscriber(pimpl_->participant_);
            pimpl_->topic_       = dds::topic::Topic<HelloWorld>(pimpl_->participant_, "HolaMundoTopic");

            pimpl_->initialized_ = true;
        } catch (const dds::core::Exception& e) {
            SYS_WARN("CycloneDDS","CycloneDDS init failed: " + std::string(e.what()) );
            return false;
        }

        // Inicializar el hilo suscriptor (recibir mensajes...)
        SYS_INFO("CycloneDDS","Initializing subscriber in a separated thread...");
        run_subscriber();

        initialized_ = true;
        return initialized_;    //<- true
    }

    bool CycloneDDS::isInitialized() const {
        return initialized_;
    }

    void CycloneDDS::loadConfig(void* config) {
        // #TODO
    }

    void CycloneDDS::close() {
        stop_subscriber();
    }


    // Ejecución ----------------------------------------------------------------------------

    void CycloneDDS::test() {

        SYS_INFO("CycloneDDS","Sending test DDS data...");

        // Preparar y enviar un mensaje
        HelloWorld msg;
        static unsigned int count = 0;
        msg.id(count);
        msg.message("Hola desde CycloneDDS (C++ API)!");
        
        if(publish_now(msg))
            SYS_INFO("CycloneDDS","Test data sent successfuly.");
        else
            SYS_WARN("CycloneDDS","Test data send error");

        count++;
        
        return;
    }

    void CycloneDDS::run_publisher() {
        // #TODO
    }

    bool CycloneDDS::publish_now(HelloWorld& msg) {

        try {
            // Crear el DataWriter
            dds::pub::DataWriter<HelloWorld> writer(pimpl_->publisher_, pimpl_->topic_);
            
            
            // Publicar el mensaje en la red
            writer.write(msg);

        } catch (const dds::core::Exception& e) {
            std::cerr << "Excepción de DDS en publisher: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    void CycloneDDS::run_subscriber() {
        if (!pimpl_->initialized_) {
            SYS_WARN("CycloneDDS","Cannot run subscriber: CycloneDDS not initialized.");
            return;
        }

        // Evitar lanzar múltiples hilos si ya hay uno ejecutándose
        if (sub_running_.load()) {
            SYS_WARN("CycloneDDS","Subscriber already running.");
            return;
        }

        sub_running_ = true;

        // Lanzamos la función de lectura dentro del std::thread miembro
        sub_thread_ = std::thread([this]() {
            SYS_INFO("CycloneDDS","Subscriber initialized");

            try {
                dds::sub::DataReader<HelloWorld> reader(pimpl_->subscriber_, pimpl_->topic_);

                // Mientras la bandera 'sub_running_' sea true, seguimos procesando
                while (sub_running_.load()) {
                    auto samples = reader.take();

                    for (const auto& sample : samples) {
                        if (sample.info().valid()) {
                            const HelloWorld& msg = sample.data();
                            std::cout << "[Recibido] ID: " << msg.id() << " | Mensaje: " << msg.message() << std::endl;
                        }
                    }

                    // Breve pausa para no saturar el hilo en polling vacíos
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            } catch (const dds::core::Exception& e) {
                SYS_WARN("CycloneDDS","Subscriber thread exception: " + std::string(e.what()));
            }

            SYS_INFO("CycloneDDS","Subscriber thread closed");
        });
    }

    void CycloneDDS::stop_subscriber() {
        // Si la bandera está en true, solicitamos la parada
        if (sub_running_.exchange(false)) {
            // Esperamos a que el hilo termine su última iteración y se cierre
            if (sub_thread_.joinable()) 
                sub_thread_.join();
            
            SYS_INFO("CycloneDDS","Subscriber stopped successfuly");
        }
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    struct CycloneDDS::Impl {};

// General ------------------------------------------------------------------------------
    CycloneDDS::CycloneDDS()  :
        initialized_(false),
        enabled_(false)
    {};
    CycloneDDS::~CycloneDDS()   {};

// Inicialización y ejecución ----------------------------------------------------------------------------
    bool CycloneDDS::init(void*)            {
        SYS_WARN("CycloneDDS", "CycloneDDS library not included");
        return false; 
    }
    bool CycloneDDS::isInitialized() const  { return false; }
    void CycloneDDS::loadConfig(void*)      { return; }
    void CycloneDDS::close()                { return; }
    void CycloneDDS::test()                 { return; }

    void CycloneDDS::run_publisher()    { return; }
    void CycloneDDS::run_subscriber()   { return; }

#endif
