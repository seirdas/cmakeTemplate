#include <filesystem>               // Controla directorios, rutas, etc.
#include <iostream>
#include "app/AppController.hpp"    // Clase controladora de aplicación
#include "system/SystemMgr.hpp"

#ifdef _WIN32
    #include <windows.h>
#endif
#include <string>

void printHelp(const std::string& app_name) {
    std::string help_msg = 
            "\nUsage: " + app_name + " [options]\n\n"
            "Options:\n"
            "  -h, --help, /h        Display this help message and exit.\n"
            "  -c, --cli, --console  Run the application in terminal/CLI mode only (no GUI).\n\n";

    #ifdef _WIN32
        bool is_attached = false;

        // Intentar vincular a la consola llamante (CMD / PowerShell)
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            is_attached = true;
        } else {
            // Si se ejecutó fuera de una consola (ej. doble clic), abrir una consola temporal
            AllocConsole();
        }

        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);

        std::cout << help_msg;

        if (is_attached) {
            // Simular la pulsación de ENTER para devolver un prompt limpio a la consola
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = VK_RETURN;
            SendInput(1, &input, sizeof(INPUT));

            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        } else {
            // Si creamos una consola nueva, pausar antes de cerrar para que dé tiempo a leer
            std::cout << "Press Enter to exit...";
            std::cin.get();
            FreeConsole();
        }
    #else
        std::cout << help_msg;
    #endif
}

int main(int argc, char** argv) {
    // Asegurar directorio del exe (para archivos de entorno de desarrollo)
    std::filesystem::path path = std::filesystem::absolute(argv[0]);
    if (std::filesystem::exists(path))
        std::filesystem::current_path(path.parent_path());

    // Comprobar argumentos de ayuda antes
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help" || arg1 == "/h" || arg1 == "/help") {
            printHelp(path.filename().string());
            return 0;
        }
    }

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
