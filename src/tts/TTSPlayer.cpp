#include "tts/TTSPlayer.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer::TTSPlayer(std::string const& playerName) :
        onTextToAudio_cb_(nullptr),
        onAudioToPlayback_cb_(nullptr),
        name_(playerName),
        initialized_(false),
        running_(false)
    {

    }
    
    /**
     * @brief Destructor 
     */
    TTSPlayer::~TTSPlayer() {
        close();
    }


// Ejecución ----------------------------------------------------------------------------

    bool TTSPlayer::init(std::string const& pbName) {
        playbackName_ = pbName;

        // Lanza el hilo que consumirá la cola interna
        data_consumer_thread_ = std::thread(&TTSPlayer::t_data_consumer, this);

        initialized_    = true;
        running_        = true;
        return initialized_;    // <- true
    }

    bool TTSPlayer::isInitialized() const {
        return initialized_;
    }

    bool TTSPlayer::close() {
        {
            std::lock_guard<std::mutex> lock(cola_textos_mtx_);
            running_ = false;
        }
        cola_textos_cv_.notify_one();
        if (data_consumer_thread_.joinable()) {
            data_consumer_thread_.join();
        }

        return true;
    }

    bool TTSPlayer::play(std::string const& modelName, std::string const& text) {

        // Comprobaciones previas
        if (modelName.empty()) {
            SYS_WARN("TTSPlayer","Cannot play: ModelName empty.");
            return false;
        }
        if (text.empty()) {
            SYS_WARN("TTSPlayer","Cannot play: Text empty.");
            return false;
        }
        if (!onTextToAudio_cb_ || !onAudioToPlayback_cb_) {
            SYS_WARN("TTSPlayer","Cannot generate audio: Callback functions don't exist.");
            return false;
        }

        std::lock_guard<std::mutex> lock(cola_textos_mtx_);
        
        // Encolamos los datos para que el hilo los procese
        queueElement element;
        element.modelName = modelName;
        element.text = text;
        cola_textos_.push(element);

        // Despertamos al hilo de procesamiento si estaba dormido
        cola_textos_cv_.notify_one();

        return true;
    }


// Datos --------------------------------------------------------------------------------

    void TTSPlayer::setPlayback(std::string const& pbName) {
        playbackName_ = pbName;
    }

    std::string TTSPlayer::getName() const {
        return name_;
    }

    std::string TTSPlayer::getPlaybackName() const {
        return playbackName_;
    }

    bool TTSPlayer::isBusy() {
        return !texto_en_proceso_.empty();
    }


// Inyección de función texto a audio ---------------------------------------------------

    void TTSPlayer::setCallback_onTextToAudio(TTSFunction fn) {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = std::move(fn); 
    }

    void TTSPlayer::clearCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = nullptr;
    }

    bool TTSPlayer::hasCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        return static_cast<bool>(onTextToAudio_cb_);
    }


// Inyección de función audio a playback ------------------------------------------------

    void TTSPlayer::setCallback_onAudioToPlayback(PlaybackFunction fn) {
        std::lock_guard<std::mutex> lk(onAudioToPlayback_mtx_);
        onAudioToPlayback_cb_ = std::move(fn);
    }

    void TTSPlayer::clearCallback_onAudioToPlayback() {
        std::lock_guard<std::mutex> lk(onAudioToPlayback_mtx_);
        onAudioToPlayback_cb_ = nullptr;
    }

    bool TTSPlayer::hasCallback_onAudioToPlayback() {
        std::lock_guard<std::mutex> lk(onAudioToPlayback_mtx_);
        return static_cast<bool>(onAudioToPlayback_cb_);
    }


// Hilos --------------------------------------------------------------------------------

    void TTSPlayer::t_data_consumer() {

        // Elemento a reproducir de la cola
        queueElement element;

        while(running_) {

            // Salir si el programa se está cerrando (antes)
            if (!running_) break;

            // Forzar espera hasta que haya algo en la cola o se cierre este player
            std::unique_lock<std::mutex> lock(cola_textos_mtx_);
            cola_textos_cv_.wait(lock, [this] {
                return !running_ || !cola_textos_.empty();
            });

            // Salir si el programa se está cerrando (después)
            if(!running_) break;

            // Obtener tarea de la cola
            element = cola_textos_.front();
            cola_textos_.pop();

            // Libera el lock de aquí en adelante
            lock.unlock();

            // Reproducir elemento
            reproducir_elemento(element);

            // Esperar un rato hasta reproducir el siguiente paquete
            std::this_thread::sleep_for(std::chrono::seconds(2));   // (poner tiempo como variable)
        }

        SYS_INFO("TTSPlayer::t_data_consumer", "Thread stopped.");

    }


// Procesado de elementos de la cola ----------------------------------------------------

    bool TTSPlayer::reproducir_elemento(queueElement element) {

        // Guardar el texto que se está procesando
        texto_en_proceso_ = element.text;

        // Generar audio
        std::vector<float> audio = onTextToAudio_cb_(element.modelName, element.text);
        if (audio.empty()) {
            SYS_WARN("TTSPlayer", "Empty audio generated from " + element.modelName);
            texto_en_proceso_.clear();
            return false;
        }

        // Reproducir audio por el playback
        bool result = onAudioToPlayback_cb_(audio, playbackName_);
        if (!result) {
            SYS_WARN("TTSPlayer", "Cannot reproduce audio through playback '" + playbackName_ + "'");
            texto_en_proceso_.clear();
            return false;
        }

        // Limpiar el texto que se está procesando
        texto_en_proceso_.clear();

        // Notificar que ha terminado de reproducir (iComm)
        /* #TODO */

        return true;
    }
