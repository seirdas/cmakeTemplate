#include <iostream>             // Entrada/Salida estándar
#include <thread>               // Hilos
#include <chrono>               // Controla tiempos de espera
#include <fstream>              // Gestiona archivos
#include <filesystem>           // Controla directorios, rutas, etc.
#include <json.hpp>             // Manipula archivos .json
#include "winMgr.h"             // Clase winMgr de gestión de ventanas
#include "UdpReceiver.hpp"      // Clase UdpReceiver para recibir datos UDP de forma asíncrona
#include "defines.h"            // Definiciones generales


int main(int /*argc*/, char** argv){

    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());



    // Ventana UI
    std::thread UIThread([]() {
        WinMgr ventana;
        if (!ventana.init())
            std::cerr << "Error al inicializar la ventana" << std::endl;
        
        while (ventana.isRunning())
            ventana.BuclePrincipal();
        
        ventana.cerrar();
        std::cout << "La ventana se ha cerrado, el hilo de UI termina ahora." << std::endl;
    });




    std::thread SocketThread([]() {
        UdpReceiver receiver("", 8080, 0);
        std::cout << "Socket UDP configurado para recibir en el puerto 8080." << std::endl;

        receiver.start();
        std::cout << "Receptor UDP iniciado, esperando paquetes..." << std::endl;

        // De momento diseñado para recibir un paquete
        //std::vector<char> packet = receiver.getNextPacket();

        // std::cout << "Paquete recibido de " << packet.size() << " bytes." << std::endl;
        // std::cout << "Contenido (hex): ";
        // for (size_t i = 0; i < std::min(packet.size(), static_cast<size_t>(16)); ++i) {
        //     std::cout << std::hex << static_cast<int>(packet[i]) << " " << std::endl;
        // }

        std::cout << "Aquí se procesaría el paquete recibido, por ejemplo pasándolo a la clase de lógica." << std::endl;

        /* Aqui la clase de la lógica ya gestiona el paquete. */
        // logicMgr.processPacket(packet);

        // Para este ejemplo, simplemente esperamos un tiempo antes de detener el receptor
        std::this_thread::sleep_for(std::chrono::seconds(10));

        receiver.stop();
        std::cout << "Receptor UDP detenido, el hilo de socket termina ahora." << std::endl;
    });
    
    // Esperar a que los hilos terminen antes de salir
    UIThread.join();
    SocketThread.join();

    return 0;
}