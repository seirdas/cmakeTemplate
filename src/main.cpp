#include <filesystem>           // Controla directorios, rutas, etc.
#include "AppController.hpp"    // Clase controladora de aplicación

int main(int argc, char** argv) {
    if (argc > 0) std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

    // Instancia controladora de la aplicación
    AppController App;
    if (App.init())
        return App.run();
    /*else*/ return -1;
}
