#include "app/AppController.hpp"
#include "system/SystemMgr.hpp"
#include <chrono>               // Controla tiempos de espera
#include <fstream>              // Para i/o de archivos
#include <memory>               // Necesario para std::unique_ptr

// General ------------------------------------------------------------------------------

AppController::AppController() :
    argc_(0),
    argv_(nullptr),
    gui_(this),
    config_filename_("config.json"),
    configJson_(nullptr),
    net_initialized_(false),
    gui_initialized_(false),
    snd_initialized_(false),
    tts_initialized_(false),
    sym_initialized_(false),
    running_(false),
    online_mode_(true),
    version_(std::to_string(VERSION))
{

}

AppController::~AppController() {

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;

    // Cerrar socket y worker esperando paquetes de red
    net_.stop();
    online_cv_.notify_all();
    SYS_INFO("AppController","Closing running threads...");
    if (hilo_consumer_.joinable())
        hilo_consumer_.join();

    if (hilo_test_.joinable())
        hilo_test_.join();

    // Cerrar módulos (opcional, recomendado)
    snd_.stop();
    gui_.cerrar();
    tts_.cerrar();

    SYS_INFO("AppController","AppController closed successfuly.");
}

bool AppController::init(int argc, char** argv) {

    SYS_INFO("AppController","Initializating application...");

    // Obtiene los parámetros de entrada
    this->argc_ = argc;
    this->argv_ = argv;

    // Guardar configuración del archivo json
    configJson_ = loadConfigFile(config_filename_);
    if (!configJson_) {
        SYS_WARN("AppController","Cannot load config file. Generating new config file.");
        if(createConfigFile(config_filename_)) {
            configJson_= loadConfigFile(config_filename_);
            if (configJson_) SYS_SOLVED("AppController","New config file created.");
        }
        else
            SYS_ERROR("AppController","Cannot create new config file. Using defaults.");
    }


    // Iniciar GUI, salir si no se carga bien
    SYS_INFO("AppController","GUI subsystem loading...");
    gui_initialized_ = gui_.init();
    if (!gui_initialized_) {
        SYS_ERROR("AppController","GUI subsystem FAIL");
        return false;   // Está diseñado para salir directamente si no hay GUI
    }
    else SYS_INFO("AppController","GUI subsystem OK");


    // Iniciar gestor de red
    SYS_INFO("AppController","Network subsystem loading...");
    net_initialized_ = net_.start();
    if(!net_initialized_)
        SYS_ERROR("AppController","Network subsystem FAIL");
    else SYS_INFO("AppController","Network subsystem OK");


    // Iniciar gestor de sonidos
    SYS_INFO("AppController","Sound subsystem loading...");
    snd_initialized_ = snd_.init();
    if(!snd_initialized_)
        SYS_ERROR("AppController","Sound subsystem FAIL");
    else SYS_INFO("AppController","Sound subsystem OK");


    // Iniciar conexión Totalmix
    SYS_INFO("AppController","Totalmix manager loading...");
    tmx_initialized_ = tmx_.init(12321,"127.0.0.1", 7001,"127.0.0.1",32,32,32);
    if(!tmx_initialized_)
        SYS_WARN("AppController","Totalmix manager FAIL");
    else SYS_INFO("AppController","Totalmix manager OK");


    // Iniciar conexión Symetrix
    SYS_INFO("AppController","Symetrix manager loading...");
    sym_initialized_ = sym_.Init(L"192.168.7.21");
    if(!sym_initialized_)
        SYS_WARN("AppController","Symetrix manager FAIL");
    else SYS_INFO("AppController","Symetrix manager OK");
    

    // Inicialización de TTS (en hilo para no bloquear)
    std::thread tLoadTTS([this]() {
            SYS_INFO("AppController","Starting TTS subsystem async load...");

            if (!configJson_) {
                SYS_WARN("AppController", "No config available for TTS, using defaults.");
                json empty = json::object();
                tts_initialized_ = tts_.init(&empty);
                return;
            }

            // Extrae la sección "tts" (o vacío si no existe)
            json tts_config = configJson_->contains("tts") ? (*configJson_)["tts"] : json::object();

            // Snapshot antes del init para detectar cambios
            json tts_config_before = tts_config;

            // Inicialización TTS
            tts_initialized_ = tts_.init(&tts_config);

            // Volcar valores sobreescritos por tts (si han cambiado)
            if (tts_config != tts_config_before) {
                SYS_INFO("AppController", "TTS config was modified by init, dumping changes...");
                std::unique_lock<std::mutex> lock(configFile_mtx_);
                (*configJson_)["tts"] = tts_config;

                // Opcional: escribir a disco
                std::ofstream file(config_filename_, std::ios::out | std::ios::trunc);
                if (file.is_open()) {
                    file << configJson_->dump(4);
                    SYS_INFO("AppController", "Config saved to " + config_filename_);
                } else {
                    SYS_WARN("AppController", "Could not save config to " + config_filename_);
                }
            }
            
        }
    );
    tLoadTTS.detach();  // No necesitamos "esperar" a que termine


    SYS_INFO("AppController","App initialized.");
    return true;
}

int AppController::run() {
    SYS_INFO("AppController","Running app...");
    running_ = true;
    
    // Hilo consumidor de paquetes online
    hilo_consumer_ = std::thread(&AppController::TWorker, this);

    // Hilo para cout de pruebas
    hilo_test_ = std::thread(&AppController::TPruebas, this);

    gui_.run(); // ← Bloquea hasta cerrar
    return 0;
}

// Configuración ------------------------------------------------------------------------

std::unique_ptr<json> AppController::loadConfigFile(std::string const& filename) {

    #if defined JSON || defined JSON_VERSION
        
        // Abrir archivo
        std::unique_lock<std::mutex> lock(configFile_mtx_);
        std::ifstream file(filename, std::ios::in);
        if (!file.is_open()) {
            SYS_WARN("AppController", "Cannot open " + filename + ".");
            return nullptr;
        }

        // Leer archivo y "almacenar" en variable json
        auto j = std::make_unique<json>();

        *j = json::parse(file);

        // Cerrar archivo (opcional)
        if (!file.is_open())
            file.close();


        // "Aprovechar" y cargar los valores miembro de AppController
        if (j) {

            // Version
            if (j->contains("version") && (*j)["version"].is_string()) {
                version_ = (*j)["version"].get<std::string>();
            } else {
                (*j)["version"] = version_; // Escribe el string por defecto en el JSON
            }

        }

        return j;

    #else
        // Devolver puntero nulo si no está disponible la librería
        return nullptr;
    #endif
}

bool AppController::createConfigFile(std::string const& filename) {
    #if defined JSON || defined JSON_VERSION
    // Crear archivo con json vacío
    std::ofstream newFile(filename, std::ios::out);
    if (!newFile.is_open()) {
        SYS_ERROR("AppController", "Cannot create " + filename + ".");
        return false;
    }
    newFile << json::object().dump(4);
    newFile.close();
    SYS_INFO("AppController", "Created empty " + filename + ".");
    return true;
    #else
         // no hacer ningún archivo si no está disponible json
        return false;
    #endif
}


// Hilos --------------------------------------------------------------------------------

void AppController::TWorker() {
    SYS_INFO("TWorker","Initializating consumer thread...");
    std::vector<char> data;

    while (running_) {

        // Forzar la espera hasta que sea notificado de un paquete nuevo
        std::unique_lock<std::mutex> lock(online_mtx_);
        online_cv_.wait(lock, [this] {
            return !running_ || online_mode_;
        });

        // Salir si el programa se está cerrando
        if (!running_) break;

        // Libera el lock de aquí en adelante
        lock.unlock();

        // Pide datos al socket para procesar
        data = net_.getNextUdpPacket();

        // Si no hay datos no hacer nada
        if (data.empty()) {
            SYS_WARN("TWorker","Empty data received");
            continue;
        }

        // Procesar el paquete (simulado)
        SYS_INFO("TWorker", "Procesando paquete de datos...");
        SYS_INFO("TWorker","Size of data " + std::to_string(data.size()));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    SYS_INFO("TWorker", "Consumer thread stopped.");
}

void AppController::TPruebas() {
    SYS_INFO("TPruebas","init TPruebas");

    // Poner aquí las pruebas o lo que sea
    /*
    while (running_) {
        SYS_INFO("TPruebas","Udp queue size: " + std::to_string(net_.numUdpRcvElements()));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    */

    SYS_INFO("TPruebas","fin TPruebas");
}


// IAppControl methods en IAppOverrides.cpp
