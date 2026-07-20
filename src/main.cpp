#include <filesystem>               // Controla directorios, rutas, etc.
#include "app/AppController.hpp"    // Clase controladora de aplicación
#include "system/SystemMgr.hpp"

int main(int argc, char** argv) {
    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    std::filesystem::path path = std::filesystem::absolute(argv[0]);
    if (std::filesystem::exists(path))
        std::filesystem::current_path(path.parent_path());

    SYS_INFO("main","Hi!");
    SYS_INFO("main","Initializing AppController...");

    // Instancia controladora de la aplicación
    AppController App;
    if (App.init(argc, argv))
        return App.run();
    else {
        SYS_ERROR("main", "The application could not be initialized.\nThe program cannot continue and will now close.");
        return -1;
    }
}
