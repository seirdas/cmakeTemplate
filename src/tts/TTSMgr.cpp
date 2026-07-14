
#include "tts/TTSMgr.hpp"
#include "tts/TTSPlayer.hpp"
#include "files/JsonMgr.hpp"
#include "Sound/SoundMgr.hpp"
#include "CLI.NET/iCommBridge.hpp"  // Puente a clase administrada (CLI.NET) iCommWrapper

#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>


// Implementación de puente para clase administrada (iComm)
struct TTSMgr::Impl {
    iCommBridge commBridge;
    Impl(TTSMgr* parent) : commBridge(parent) {}
};


// General ------------------------------------------------------------------------------

TTSMgr::TTSMgr(SoundMgr* snd) :
    pimpl_(std::make_unique<Impl>(this)),
    initialized_(false),
    running_(false),
    snd_(snd)
{

}

TTSMgr::~TTSMgr() {
    cerrar();
}


// Ejecución ----------------------------------------------------------------------------

bool TTSMgr::init(void* config) {

    // Validar y asignar valores de variables miembro a partir de la config pasada (json)
    if (config)
        loadConfig(config);
    else
        SYS_WARN("TTSMgr","Cannot load config. Using default values.");
    

    // Inicialización de iComm (CLI.NET)
    SYS_INFO("TTSMgr","Starting iComm (.NET) client...");
    if(!pimpl_->commBridge.init())
        SYS_WARN("TTSMgr","iComm FAIL");
    else SYS_INFO("TTSMgr","iComm OK");


    // Inicialización de TTSCore (en hilo para no bloquear)
    SYS_INFO("TTSMgr","Starting TTSCore async load...");
    hilo_ttscore_ = std::thread([this, config]() {
            if(!ttsCore_.init(config))
                SYS_WARN("TTSMgr","TTSCore FAIL");
            else SYS_INFO("TTSMgr","TTSCore OK");
            JsonMgr::instance().update();
        }
    );
    

    // Hilo consumidor de paquetes TTS
    hilo_consumer_ = std::thread(&TTSMgr::TWorker, this);

    
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

    SYS_INFO("TTSMgr","Reading config node...");

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    // Cargar TTSPlayers a partir del json
    // #TODO

    SYS_INFO("TTSMgr","Config node read OK");
}

void TTSMgr::cerrar() {

    // Si no está corriendo, no hacer nada
    if (!running_)
        return;

    // Notifica el estado de cerrado (para threads, etc.)
    running_ = false;
    queue_cv_.notify_all();

    // Cierra el cliente de iComm
    SYS_INFO("TTSMgr","Closing iComm (.NET) client...");
    if(!pimpl_->commBridge.close())
        SYS_WARN("TTSMgr","Closing iComm FAIL");

    // Cierra el núcleo de TTS
    SYS_INFO("TTSMgr","Closing ttsCore...");
    ttsCore_.cerrar();

    // Espera a que se cierre el hilo consumidor
    if (hilo_consumer_.joinable()) {
        SYS_INFO("TTSMgr","Waiting for consumer thread...");
        hilo_consumer_.join();
    }

    // Espera a que se cierre el hilo que inicia el TTSCore (si aplica)
    if (hilo_ttscore_.joinable()) {
        SYS_INFO("TTSMgr","Waiting for TTSCore thread...");
        hilo_ttscore_.join();
    }

    SYS_INFO("TTSMgr","TTSMgr closed successfuly");
}

void TTSMgr::Ejecutar(const TTSDataPacket& packet) {

    // Añadir paquete a la queue
    /* TBD */

    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        queue_.push(packet);
    }
    // Avisar al worker de que hay paquete
    queue_cv_.notify_one();
}


// TTSCore ------------------------------------------------------------------------------

bool TTSMgr::generateWav(std::string const& modelName, std::string const& text, std::string wavname) {
    return ttsCore_.generateWav(modelName, text, wavname);
}

std::vector<std::string> TTSMgr::getAvailableModels() {
    return ttsCore_.getAvailableModels();
}

std::vector<std::string> TTSMgr::getLoadedModels() const {
    return ttsCore_.getLoadedModels();
}

short TTSMgr::numLoadedModels() const {
    return ttsCore_.numLoadedModels();
}

short TTSMgr::numAvailableModels() const {
    return ttsCore_.numAvailableModels();
}


// Gestión de reproductores TTS ---------------------------------------------------------

bool TTSMgr::add_tts_player(std::string const& TTSPlayerName, std::string const& playbackName) {
    std::lock_guard<std::mutex> lock(queue_mtx_);
    
    if (ttsPlayers_.find(TTSPlayerName) != ttsPlayers_.end()) {
        return false; // Ya existe un reproductor para ese canal/dispositivo
    }

    // No interesa crear nada si no hay módulo de sonidos para asignar playbacks
    if (!snd_) {
        SYS_WARN("TTSMgr","Cannot create TTSPlayer: Sound module not defined");
        return false;
    }

    // Creamos el reproductor dedicado
    std::unique_ptr<TTSPlayer> player = std::make_unique<TTSPlayer>();
    player->init(TTSPlayerName);

    // Establece el dispositivo de reproducción si se ha indicado
    if (!playbackName.empty())
        player->setPlaybackDev(playbackName);

    // INYECCIÓN: Generar audio del texto usando TTSCore
    player->setTTSCallback([this](std::string const& modelName, std::string const& text) -> std::vector<float> {
        AudioData data = ttsCore_.generate(modelName, text);
        return data.samples;
    });

    // INYECCIÓN: Reproducir el audio usando SoundMgr
    player->setPlaybackCallback([this](std::vector<float>& audio, const std::string& playbackName) -> bool {
        if (!snd_) {
            SYS_WARN("TTSPlayer","Cannot reproduce audio: Sound module not defined");
            return false;
        }

        // #TODO
    });

    // Guardar en TTSPlayer en la lista de TTSMgr
    ttsPlayers_[TTSPlayerName] = std::move(player);
    return true;
}

bool TTSMgr::remove_tts_player(std::string const& name) {
    std::lock_guard<std::mutex> lock(queue_mtx_);
    return ttsPlayers_.erase(name) > 0;
}


// TTSPlayer ----------------------------------------------------------------------------

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

    // Comprobar si en el playbackName se puede reproducir
    if (!snd_->isOnManagedCaptures(playbackName)) {
        SYS_WARN("TTSMgr","Play error: Playback device is not managed");
        return false;
    }

    //std::unique_lock<std::mutex> lock(queue_mtx_);

    // Comprobar si la entidad existe ya en la info de algún player
    TTSMgrInfo* myinfo = nullptr;
    for (auto& it : ttsPlayers_) {
        for (auto& info : PlayersInfo_[it.first])
            if(entityName == info.entity_name) {
                myinfo = &info;
                break;
            }
        if (myinfo) break;
    }

    // Si se ha definido un nuevo modelo de voz, se asigna (exista o no la info)
    if (!modelName.empty()) {
        if (!ttsCore_.isModelLoaded(modelName)) {
            SYS_WARN("TTSMgr","Play error: Model selected doesn't exist");
            return false;
        }
        myinfo->model_name_assigned = modelName;
    }

    // Caso cuando no existe la info (info nueva)
    if (!myinfo) {
        TTSMgrInfo newInfo;
        newInfo.entity_name = entityName;

        // Si no se ha definido un modelo de voz, elegir uno cualquiera
        if (modelName.empty()) {
            std::vector<std::string> models = ttsCore_.getLoadedModels();
            if (models.empty()) {
                SYS_WARN("TTSMgr","Play error: Cannot gather any TTS model.");
                return false;
            }
            unsigned short sel = rand() % models.size();
            myinfo->model_name_assigned = models[sel];
        }

        // Guardar la entidad en la info
        newInfo.entity_name = entityName;
    }

    // Desbloqueo de mutex antes de reproducir para evitar deadlocks
    //lock.unlock();
    
    // Asignar a un TTSPlayer...
    /* #TODO */

    // Callback para mostrar texto mientras se está reproduciendo, quitar después
    /* #TODO */

    SYS_WARN("TTSMgr","Not yet implemented");
    return false;
}


// Hilos --------------------------------------------------------------------------------

void TTSMgr::TWorker() {

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
