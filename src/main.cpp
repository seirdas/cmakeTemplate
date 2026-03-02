
#include <filesystem>           // Controla directorios, rutas, etc.
#include "AppController.hpp"    // Clase controladora de aplicación

int main(int /*argc*/, char** argv){

    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    auto path = std::filesystem::absolute(argv[0]);
    if (std::filesystem::exists(path))
        std::filesystem::current_path(path.parent_path());

    // Instancia controladora de la aplicación
    AppController App;
    if (App.init())
        return App.run();
    /*else*/ return -1;
}
