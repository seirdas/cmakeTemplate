#include "tts/TTSPlayer.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor 
     */
    TTSPlayer::TTSPlayer(std::string const& playerName) :
        fn_textToAudio(nullptr),
        fn_audioToPlayback(nullptr),
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
        worker_thread_ = std::thread(&TTSPlayer::TProcesarCola, this);

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
        if (worker_thread_.joinable()) {
            worker_thread_.join();
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
        if (!fn_textToAudio || !fn_audioToPlayback) {
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

    std::string TTSPlayer::getPlaybackName() const {
        return playbackName_;
    }

    bool TTSPlayer::isBusy() {
        return !texto_en_proceso_.empty();
    }


// Inyección de funciones ---------------------------------------------------------------

    void TTSPlayer::setTTSCallback(TTSFunction fn) {
        fn_textToAudio = std::move(fn); 
    }

    void TTSPlayer::setPlaybackCallback(PlaybackFunction fn) {
        fn_audioToPlayback = std::move(fn);
    }


// Hilos --------------------------------------------------------------------------------

    void TTSPlayer::TProcesarCola() {

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
            reproducirElemento(element);

            // Esperar un rato hasta reproducir el siguiente paquete
            std::this_thread::sleep_for(std::chrono::seconds(2));   // (poner tiempo como variable)
        }

        SYS_INFO("TTSPlayer::TProcesarCola", "Thread stopped.");

    }


// Procesado de elementos de la cola ----------------------------------------------------

    bool TTSPlayer::reproducirElemento(queueElement element) {

        // Guardar el texto que se está procesando
        texto_en_proceso_ = element.text;

        // Generar audio
        std::vector<float> audio = fn_textToAudio(element.modelName, element.text);
        if (audio.empty()) {
            SYS_WARN("TTSPlayer", "Empty audio generated from " + element.modelName);
            texto_en_proceso_.clear();
            return false;
        }

        // Reproducir audio por el playback
        bool result = fn_audioToPlayback(audio, playbackName_);
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
