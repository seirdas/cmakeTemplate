#include <iostream>
#include <thread>               // Hilos
#include <chrono>   
#include <fstream>              // Gestiona archivos
#include <filesystem>           // Controla directorios, rutas, etc.
#include <nlohmann/json.hpp>    // Manipula archivos .json
#include "defines.h"
#include "winMgr.h"





int main(int /*argc*/, char** argv){

    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());
    // Solo escribir en consola si hay terminal asociada
    #ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            // Código ANSI para limpiar pantalla y resetear cursor
            std::cout << "\033[2J\033[1;1H";
        }
    #endif



    WinMgr ventana;
    if (!ventana.init())
        return -1;
    
    while (ventana.isRunning())
        ventana.frame();
    
    ventana.close();




    // TEST: Hilos
    std::thread worker([]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Mensaje: el hilo ha esperado 3 segundos y termina ahora." << std::endl;
    });
    
    // Esperar a que el hilo termine antes de salir
    worker.join();

    return 0;
}