#include <iostream>             // Entrada/Salida estándar
#include <thread>               // Hilos
#include <chrono>               // Controla tiempos de espera
#include <fstream>              // Gestiona archivos
#include <filesystem>           // Controla directorios, rutas, etc.
#include <atomic>               // Variables atómicas para control de hilos
#include <json.hpp>             // Manipula archivos .json
#include "winMgr.h"             // Clase winMgr de gestión de ventanas
#include "UdpReceiver.hpp"      // Clase UdpReceiver para recibir datos UDP de forma asíncrona
#include "defines.h"            // Definiciones generales


int main(int /*argc*/, char** argv){

    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    auto path = std::filesystem::absolute(argv[0]);
    if (std::filesystem::exists(path))
        std::filesystem::current_path(path.parent_path());


    // Socket
    std::cout << "Inicializando socket..." << std::endl;
    UdpReceiver receiver(8080);
    receiver.start();
    
    // Ventana UI
    WinMgr ventana;
    
    
    
    // Hilo para la ventana de UI
    std::thread UIThread(
        [&receiver, &ventana]() {

            std::cout << "Inicializando UI..." << std::endl;

            if (!ventana.init())
                std::cerr << "Error al inicializar la ventana" << std::endl;
            
            while (ventana.isRunning())
                ventana.BuclePrincipal();
            
            // Si se cierra la ventana, cerramos socket
            receiver.stop();

            ventana.cerrar();   // No haría falta porque el destructor ya lo hace

            std::cout << "La ventana se ha cerrado, el hilo de UI termina ahora." << std::endl;
        }
    );
    
    

    while(receiver.isRunning()) {
        std::vector<char> packet = receiver.getFirstPacket();
        
        // Ha llegado un paquete vacío. Check si sigue en ejecución o se ha cerrado el receptor. 
        if (packet.empty()) {
            if (!receiver.isRunning()) break;       // Sale del bucle si el receptor se ha cerrado
            else continue;                          // Si el receptor sigue en ejecución, sigue esperando paquetes
        }
        
        std::cout << "Procesando paquete..." << std::endl;
        
        /* Aqui la clase de la lógica ya gestiona el paquete. */                
        // Para este ejemplo, simplemente esperamos un tiempo antes de detener el receptor
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    
    // Esperar a que los hilos terminen antes de salir
    if (UIThread.joinable()) UIThread.join();

    return 0;
}