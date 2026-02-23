#include "AppController.hpp"
#include <chrono>               // Controla tiempos de espera

/********************************/
// ----- APPCONTROLLER ----------/
/********************************/

AppController::AppController() : ui_(this) {

}

AppController::~AppController() {
    
    // Cerrar sockets
    std::cout << "Cerrando receiver1..." << std::endl;
    if (receiver1_.isRunning())
        receiver1_.stop();
    
    std::cout << "Cerrando receiver2..." << std::endl;
    if (receiver2_.isRunning())
        receiver2_.stop();
    
    // Cerrar ventana UI
    std::cout << "Cerrando ventana UI..." << std::endl;
    ui_.cerrar();

}

bool AppController::init(){

    // Inicialización de socket de recepción 1.
    std::cout << "Inicializando socket 1..." << std::endl;
    if (!receiver1_.start(12345)){
        std::cerr << "Error al inicializar el socket 1" << std::endl;
        return false;
    }
    std::cout << "Socket inicializado en puerto " << receiver1_.getLocalPort() << std::endl;
    

    // Inicialización de socket de recepción 2.
    std::cout << "Inicializando socket 2..." << std::endl;
    if (!receiver2_.start(12345)){
        std::cerr << "Error al inicializar el socket 1" << std::endl;
        return false;
    }
    std::cout << "Socket inicializado en puerto " << receiver2_.getLocalPort() << std::endl;

    // Inicialización de ventana UI
    if (!ui_.init()) 
        return false;


    /*else*/
    return true;
}

int AppController::run() {

    // De momento no gestiono los paquetes recibidos por sockets

    // Ventana UI
    ui_.run();      // <-- Este método bloquea hasta que la ventana se cierre

    return 0;
}


// Función temporal guardado de gestión del paquete socket
// Pendiente de implementar
void ejecutar_paquete(){
    // Variable para notificar al hilo de lógica que la aplicación se está cerrando
    std::atomic<bool> app_running{true};

    // Temporal para evitar errores
    UdpReceiver receiver_;

    // Creamos un hilo para vaciar la cola del receptor mientras la UI corre
    std::thread logicThread(
        [&]() {
            std::this_thread::sleep_for(std::chrono::seconds(4)); // Espera a que la UI esté lista
            do {
                std::vector<char> packet = receiver_.getFirstPacket();
                
                // Si ha llegado un paquete vacío, check si sigue en ejecución o se ha cerrado el receptor. 
                if (packet.empty()) {
                    if (!receiver_.isRunning()) break;      // Sale del bucle si el receptor se ha cerrado
                    else continue;                          // Si el receptor sigue en ejecución, sigue esperando paquetes
                }
                
                // Procesar lógica aquí
                std::cout << "Paquete de " << packet.size() << " bytes recibido." << std::endl;

                /* Aqui la clase de la lógica ya gestiona el paquete. */                
                // Para este ejemplo, simplemente esperamos un tiempo antes de detener el receptor
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

            } while (app_running && receiver_.isRunning());
            std::cout << "Lógica del hilo terminada." << std::endl;
        }
    );

    /* La función se quedaría aquí hasta cerrar */
    //ui_.run();

    app_running = false;
    logicThread.join();
}