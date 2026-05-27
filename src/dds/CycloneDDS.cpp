#include "dds/CycloneDDS.hpp"
#include "system/SystemMgr.hpp"
#include <thread>

#if defined CYCLONEDDSCXX || defined CYCLONEDDSCXX_VERSION

    /* ---- ES NORMAL QUE SALGAN ERRORES: */
    // Se necesita al menos compilar una vez para CYCLONEDDS se instale 
    // en el proyecto y no aparezcan los errores en los componentes de cyclonedds
    // Si aun así salen errores, borrar la carpeta '_build' y reconfigurar de nuevo

    #include <iostream>
    #include "dds/dds.hpp"

    // Include de los archivos .idl en formato .hpp generados por cyclone
    #include "cyclone_generated/_ALL.hpp"

    // General ------------------------------------------------------------------------------

    CycloneDDS::CycloneDDS() {

    }

    CycloneDDS::~CycloneDDS() {

    }

    void CycloneDDS::test() {
        
        SYS_INFO("CycloneDDS","Start CycloneDDS node");

        // 1. Crear el Participante (Se une al dominio por defecto 0)
        dds::domain::DomainParticipant participant(0);

        // 2. Crear el Tópico usando    las clases generadas por C++
        dds::topic::Topic<HelloWorld> topic(participant, "HolaMundoTopic");

        // 3. Crear el Publisher y el DataWriter
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<HelloWorld> writer(publisher, topic);

        std::cout << "--- Nodo listo. Enviando datos... ---" << std::endl;

        // 4. Preparar y enviar un mensaje usando los Setters de C++
        HelloWorld msg;
        msg.id(1);
        msg.message("Hola desde CycloneDDS (C++ API)!");

        // El envío es asíncrono
        writer.write(msg);

        std::cout << "[DDS] Mensaje de test enviado correctamente." << std::endl;
        
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

    CycloneDDS::CycloneDDS()    {};
    CycloneDDS::~CycloneDDS()   {};

    void CycloneDDS::test()     { return; };

    void CycloneDDS::run_publisher()    { return; }
    void CycloneDDS::run_subscriber()   { return; }

#endif
