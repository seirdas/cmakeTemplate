#pragma once

#include "dds/dds.h"
#include <iostream>
#include "net/idl.h"

class ddsModule {

public:
    ddsModule();
    ~ddsModule();

    void test() {
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
        
        return 0;
    }


private:

    /* Variables */


};