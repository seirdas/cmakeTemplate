



/* !!!!!!!!!!! ATENCION !!!!!!!!!!!!
* ESTOY SUSTITUYENDO ESTA CLASE POR TTSDISPATCHER
* NO TENER EN CUENTA NADA DE LO QUE HAY AQUÍ 
*/



#include "tts_obsoleto/TTSMgr.hpp"
#include "files/JsonMgr.hpp"
#include "sound/SoundMgr.hpp"
#include "system/SystemMgr.hpp"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>


// General ------------------------------------------------------------------------------

TTSMgr::TTSMgr() :
    initialized_(false),
    running_(false)
{

}

TTSMgr::~TTSMgr() {
    close();
}


// Ejecución ----------------------------------------------------------------------------

bool TTSMgr::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else
        SYS_WARN("TTSMgr","Cannot load config. Using default values.");
    
    // Inicialización de TTSCore (en hilo para no bloquear)
    SYS_INFO("TTSMgr","Starting TTSCore async load...");
    initTTS_thread_ = std::thread([this, config]() {

            // Inyectar función de notify al TTSCore
            ttsCore_.setCallback_onNotify([this](){
                this->notify();
            });

            // Inicializar submódulo 
            if(!ttsCore_.init(config))
                SYS_WARN("TTSMgr","TTSCore FAIL");
            else SYS_INFO("TTSMgr","TTSCore OK");

            // Actualizar json con los datos de ttscore (si aplica)
            JsonMgr::instance().update();
        }
    );
    

    // Hilo consumidor de paquetes TTS
    dataConsumer_thread_ = std::thread(&TTSMgr::t_data_consumer, this);

    
    // Marcar el módulo internamente como inicializado y corriendo
    initialized_    = true;
    running_        = true;

    return initialized_;    // <- true
}

bool TTSMgr::isInitialized() const {
    return initialized_;
}

void TTSMgr::loadConfig(void* config) {

    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // Cargar TTSPlayers a partir del json
    // #TODO

}

void TTSMgr::close() {

    // Si no está corriendo, no hacer nada
    if (!running_)
        return;

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;
    queue_cv_.notify_all();

    // Espera a que se cierre el hilo consumidor
    if (dataConsumer_thread_.joinable()) {
        SYS_INFO("TTSMgr","Waiting for consumer thread...");
        dataConsumer_thread_.join();
    }

    // Espera a que se cierre el hilo que inicia el TTSCore (si aplica)
    if (initTTS_thread_.joinable()) {
        SYS_INFO("TTSMgr","Waiting for TTSCore thread...");
        initTTS_thread_.join();
    }

    SYS_INFO("TTSMgr","TTSMgr closed successfully");
}


// PlayerTTS ----------------------------------------------------------------------------

bool TTSMgr::play(
    std::string const& text, 
    std::string const& entityName,
    std::string const& modelName, 
    std::string const& playbackName) 
{
    // Comprobaciones
    if (text.empty()) {
        SYS_WARN("TTSMgr","Play error: text empty");
        return false;
    }

    //std::unique_lock<std::mutex> lock(queue_mtx_);

    // // Comprobar si la entidad existe ya en la info de algún player
    // TTSMgrInfo* myinfo = nullptr;
    // for (auto& it : ttsPlayers_) {
    //     for (auto& info : PlayersInfo_[it.first])
    //         if(entityName == info.entityName) {
    //             myinfo = &info;
    //             break;
    //         }
    //     if (myinfo) break;
    // }

    // // Si se ha definido un nuevo modelo de voz, se asigna (exista o no la info)
    // if (!modelName.empty()) {
    //     if (!ttsCore_.isModelLoaded(modelName)) {
    //         SYS_WARN("TTSMgr","Play error: Model selected doesn't exist");
    //         return false;
    //     }
    //     /* #TODO */
    //     //myinfo->model_name_assigned = modelName;
    // }

    // // Caso cuando no existe la info (info nueva)
    // if (!myinfo) {
    //     TTSMgrInfo newInfo;
    //     newInfo.entityName = entityName;

    //     // Si no se ha definido un modelo de voz, elegir uno cualquiera
    //     if (modelName.empty()) {
    //         std::vector<std::string> models = ttsCore_.getLoadedModels();
    //         if (models.empty()) {
    //             SYS_WARN("TTSMgr","Play error: Cannot gather any TTS model.");
    //             return false;
    //         }
    //         unsigned short sel = rand() % models.size();
    //         /* #TODO */
    //         //myinfo->model_name_assigned = modelName;
    //     }

    //     // Guardar la entidad en la info
    //     newInfo.entityName = entityName;
    // }

    // Desbloqueo de mutex antes de reproducir para evitar deadlocks
    //lock.unlock();
    
    // Asignar a un PlayerTTS...
    /* #TODO */

    // Callback para mostrar texto mientras se está reproduciendo, quitar después
    /* #TODO */

    SYS_WARN("TTSMgr","Not yet implemented");
    return false;
}


// Observadores -------------------------------------------------------------------------

void TTSMgr::addObserver(ITTSObserver* obs) {
    observers_.push_back(obs);
}


// Notificar a observadores -------------------------------------------------------------

void TTSMgr::notify() {
    // 1. Empaquetamos los datos actuales
    TTSCoreData data;
    data.available_models       = ttsCore_.getAvailableModels();
    data.loaded_models          = ttsCore_.getLoadedModels();
    data.num_available_models   = ttsCore_.numAvailableModels();
    data.num_loaded_models      = ttsCore_.numLoadedModels();
    
    if (data.num_available_models > 0) {
        data.init_percent = static_cast<short>(100.0 * data.num_loaded_models / data.num_available_models);
    }

    // 2. Notificamos a los observadores pasándoles la estructura
    for (auto* obs : observers_) {
        obs->onTTSDataChanged(data);
    }
}


// Hilos --------------------------------------------------------------------------------

void TTSMgr::t_data_consumer() {

    while (running_) {

        // Salir si el programa se está cerrando (antes de bloqueo)
        if (!running_)
            break;

        // Forzar la espera hasta que sea notificado de un paquete nuevo
        std::unique_lock<std::mutex> lock(queue_mtx_);
        queue_cv_.wait(lock, [this] {
            return !running_ || !queue_.empty();
        });

        // Salir si el programa se está cerrando (después de bloqueo)
        if (!running_)
            break;

        // Va consumiendo la cola de datos pasándoselo a los ttsPlayers
        /* #TODO */

        // Lógica de ejemplo (borrar al implementar la de verdad)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }
}

