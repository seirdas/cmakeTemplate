#include "sound/PlayerTTS.hpp"
#include "system/SystemMgr.hpp"


// General ------------------------------------------------------------------------------

    PlayerTTS::PlayerTTS(std::string const& moduleName, void* ctx, const void* device_info) :
        AudioPlaybackModule(moduleName, ctx, device_info),
        onTextToAudio_cb_(nullptr)
    {

    }


// Ejecución ----------------------------------------------------------------------------

    bool PlayerTTS::playTTS(std::string const& modelName, std::string const& text) {

        // Comprobaciones previas
        if (modelName.empty()) {
            SYS_WARN("PlayerTTS","Cannot play: ModelName empty.");
            return false;
        }
        if (text.empty()) {
            SYS_WARN("PlayerTTS","Cannot play: Text empty.");
            return false;
        }
        if (!onTextToAudio_cb_) {
            SYS_WARN("PlayerTTS","Cannot generate audio: Callback functions don't exist.");
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


// Inyección de función texto a audio ---------------------------------------------------

    void PlayerTTS::setCallback_onTextToAudio(TTSFunction fn) {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = std::move(fn); 
    }

    void PlayerTTS::clearCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        onTextToAudio_cb_ = nullptr;
    }

    bool PlayerTTS::hasCallback_onTextToAudio() {
        std::lock_guard<std::mutex> lk(onTextToAudio_mtx_);
        return static_cast<bool>(onTextToAudio_cb_);
    }


// Hilos --------------------------------------------------------------------------------

    void PlayerTTS::t_data_consumer() {

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

        SYS_INFO("PlayerTTS::t_data_consumer", "Thread stopped.");

    }


// Procesado de elementos de la cola ----------------------------------------------------

    bool PlayerTTS::reproducir_elemento(queueElement element) {

        // Guardar el texto que se está procesando
        texto_en_proceso_ = element.text;

        // Generar audio
        std::vector<float> audio = onTextToAudio_cb_(element.modelName, element.text);
        if (audio.empty()) {
            SYS_WARN("PlayerTTS", "Empty audio generated from " + element.modelName);
            texto_en_proceso_.clear();
            return false;
        }

        // Reproducir audio por el playback
        
        // #TODO

        // Limpiar el texto que se está procesando
        texto_en_proceso_.clear();

        // Notificar que ha terminado de reproducir (iComm)
        /* #TODO */

        return true;
    }
