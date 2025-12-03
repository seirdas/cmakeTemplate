#include <iostream>
#include <thread>
#include <chrono>
#include "functions.h"

int main(){
    std::cout << "Hello" << myFunc() << std::endl;

    // Hilo que espera 5 segundos, muestra un mensaje y termina
    std::thread worker([]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Mensaje: el hilo ha esperado 5 segundos y termina ahora." << std::endl;
    });

    // Esperar a que el hilo termine antes de salir
    worker.join();

    return 4;
}