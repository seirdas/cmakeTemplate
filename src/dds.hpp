// Cabecera principal del estándar DDS en C++
#include <dds/dds.hpp>
#include <dds/dds.h>

// Tu archivo generado por el compilador idlc
#include "idl_data.hpp"

// ==========================================================
// FUNCIÓN PARA ENVIAR DATOS (PUBLISHER)
// ==========================================================
void run_publisher() {
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

// ==========================================================
// FUNCIÓN PARA RECIBIR DATOS (SUBSCRIBER)
// ==========================================================
void run_subscriber() {
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
