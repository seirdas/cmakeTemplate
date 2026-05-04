#include <filesystem>           // Controla directorios, rutas, etc.
#include "AppController.hpp"    // Clase controladora de aplicación

#include "dds.hpp"

int main(int argc, char** argv) {
    if (argc > 0) std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());


    /* Prueba cyclonedds */

    // Leer el argumento de la consola para decidir qué rol tomar
    std::string mode = "pub"; // Por defecto es publicador

    if (argc > 1)
        mode = argv[1];

    if (mode == "sub")
        run_subscriber();       // <- bloqueante
    else if (mode == "pub")
        run_publisher();        // <- bloqueante





    // Instancia controladora de la aplicación
    AppController App;
    if (App.init())
        return App.run();
    /*else*/ return -1;
}
