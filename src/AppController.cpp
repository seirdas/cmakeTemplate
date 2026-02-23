#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera

/********************************/
// ----- APPCONTROLLER ----------/
/********************************/

AppController::AppController() : ui_(this) {

}

AppController::~AppController() {
    
    // Cerrar sockets
    std::cout << "Cerrando sockets..." << std::endl;
    net_.stop();
    
    // Cerrar ventana UI
    std::cout << "Cerrando ventana UI..." << std::endl;
    ui_.cerrar();

}

bool AppController::init(){

    // Inicialización de sockets
    net_.addReceiver(8080);
    net_.addReceiver(12345, "127.0.0.1", sizeof(unsigned long long));

    // Inicialización de ventana UI
    if (!ui_.init()) 
        return false;

    /*else*/
    return true;
}

int AppController::run() {

    
    // Iniciar sockets
    net_.start();

    // De momento no gestiono los paquetes recibidos por sockets

    // Ventana UI
    ui_.run();      // <-- Este método bloquea hasta que la ventana se cierre

    return 0;
}