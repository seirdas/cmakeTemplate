#include "dds/CycloneDDS.hpp"

#ifdef USE_CYCLONEDDSCXX

    #include <iostream>
    #include "dds/dds.h"
    #include "cyclone_generated/idl_data.h"

    // General ------------------------------------------------------------------------------

    CycloneDDS::CycloneDDS() {

    }

    CycloneDDS::~CycloneDDS() {

    }

    void CycloneDDS::test() {
        dds_entity_t participant;
        dds_entity_t topic;
        dds_entity_t writer;
        dds_return_t rc;
        
        HelloWorld msg; // Nuestra estructura definida en el IDL
        void *samples[1];

        std::cout << "--- Iniciando Nodo CycloneDDS ---" << std::endl;

        // 1. Crear el Participante (Se une al dominio por defecto 0)
        participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
        if (participant < 0) {
            DDS_FATAL("dds_create_participant: %s\n", dds_strretcode(-participant));
        }

        // 2. Crear el Tópico
        // 'HelloWorld_desc' es una estructura generada por el IDL que describe el tipo
        topic = dds_create_topic(participant, &HelloWorld_desc, "HolaMundoTopic", NULL, NULL);

        // 3. Crear el Publisher/Writer (Para enviar datos)
        writer = dds_create_writer(participant, topic, NULL, NULL);

        std::cout << "--- Nodo listo. Enviando datos... ---" << std::endl;

        // 4. Preparar y enviar un mensaje
        msg.id = 1;
        msg.message = (char*)"Hola desde CycloneDDS!";
        samples[0] = &msg;

        // El envío es asíncrono y automático para cualquier "Subscriber" en la red
        rc = dds_write(writer, samples[0]);

        if (rc >= 0) {
            std::cout << "[DDS] Mensaje enviado correctamente." << std::endl;
        }

        // 5. Limpieza
        rc = dds_delete(participant);
        
        return;
    }


    void CycloneDDS::run_publisher() {
        std::cout << "--- Iniciando Publisher ---" << std::endl;

        try {
            // 1. Crear el Participante en el Dominio por defecto (0)
            dds::domain::DomainParticipant participant(0);

            // 2. Crear el Topic asociado al tipo generado "HelloWorld"
            dds::topic::Topic<HelloWorld> topic(participant, "HelloWorldTopic");

            // 3. Crear el Publisher y el DataWriter
            dds::pub::Publisher publisher(participant);
            dds::pub::DataWriter<HelloWorld> writer(publisher, topic);

            int counter = 0;
            
            // 4. Bucle de envío
            while (true) {
                HelloWorld msg;
                // Usamos los setters generados por idlc
                msg.id(counter);
                msg.message("Mensaje de prueba desde CycloneDDS! #" + std::to_string(counter));

                std::cout << "[Enviando] ID: " << msg.id() << " | Mensaje: " << msg.message() << std::endl;
                
                // Publicar el mensaje en la red
                writer.write(msg);

                counter++;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const dds::core::Exception& e) {
            std::cerr << "Excepción de DDS en publisher: " << e.what() << std::endl;
        }
    }

    void CycloneDDS::run_subscriber() {
        std::cout << "--- Iniciando Subscriber ---" << std::endl;

        try {
            // 1. Crear el Participante en el mismo Dominio (0)
            dds::domain::DomainParticipant participant(0);

            // 2. Crear el Topic (debe llamarse igual que en el publisher)
            dds::topic::Topic<HelloWorld> topic(participant, "HelloWorldTopic");

            // 3. Crear el Subscriber y el DataReader
            dds::sub::Subscriber subscriber(participant);
            dds::sub::DataReader<HelloWorld> reader(subscriber, topic);

            std::cout << "Esperando mensajes..." << std::endl;

            // 4. Bucle de lectura (Polling simple)
            while (true) {
                // take() extrae las muestras y las borra de la cola del lector
                // read() solo las leería sin borrarlas
                auto samples = reader.take();

                for (const auto& sample : samples) {
                    // Hay que comprobar si la muestra es válida (a veces DDS manda meta-eventos)
                    if (sample.info().valid()) {
                        const HelloWorld& msg = sample.data();
                        std::cout << "[Recibido] ID: " << msg.id() << " | Mensaje: " << msg.message() << std::endl;
                    }
                }

                // Pausa breve para no saturar la CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const dds::core::Exception& e) {
            std::cerr << "Excepción de DDS en subscriber: " << e.what() << std::endl;
        }
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    CycloneDDS::CycloneDDS()    = default;
    CycloneDDS::~CycloneDDS()   = default;

    void CycloneDDS::test()     { return; };

    void CycloneDDS::run_publisher()    { return; }
    void CycloneDDS::run_subscriber()   { return; }

#endif
