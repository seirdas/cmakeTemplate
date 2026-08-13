#include "sound/AudioPlaybackModule.hpp"


#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include <miniaudio.h>
    #include "system/SystemMgr.hpp"
    #include "datatypes/MorseDict.hpp"

    #include <memory>
    #include <unordered_map>
    #include <filesystem>           // Controla directorios, rutas, etc.
    #include <mutex>
    #include <queue>
    #include <string>
    #include <thread>


    // Implementación de miembros y métodos de la librería externa
    struct AudioPlaybackModule::Impl {


        // Estructuras y enumerados

        /** @brief Elemento de audio en reproducción */
        struct SoundInstance {
            ma_sound        sound;              ///< La instancia del sonido en mini audio
            std::string     name;               ///< Nombre del audio
            bool            loopMode   = false; ///< Indica reproducción en bucle del sonido
            bool            forceStop  = false; ///< Indica si el sonido debe pararse sin acabarlo completamente
            ma_audio_buffer buffer;             ///< Buffer en memoria (solo si el sonido viene de un morse, no de archivo)
            bool            isBuffer = false;   ///< Indica si "buffer" está inicializado y hay que liberarlo
        };

        /** @brief Elemento de caché de archivo de audio */
        struct AudioCacheInstance {
            ma_sound    sound;                              ///< Archivo de audio cacheado
            std::chrono::steady_clock::time_point time;     ///< Marca de tiempo en el que se cacheó el audio
        };


        // Aliases
        using SoundList     = std::unordered_map<std::string, std::unique_ptr<SoundInstance>>;
        using CacheList     = std::unordered_map<std::string, std::unique_ptr<AudioCacheInstance>>;
        using ColaCleanup   = std::queue<std::unique_ptr<SoundInstance>>;

        // Componentes de miniaudio
        ma_context*     ctx;                ///< Contexto de miniaudio
        ma_device_info  device_info;        ///< Información del dispositivo de audio
        ma_engine       engine;             ///< Motor de audio

        // Listas de sonidos
        SoundList       playing_sounds;     ///< Mapa de instancias de sonido reproduciéndose (nombreWav -> SoundInstance)
        CacheList       sounds_cache;       ///< Mapa de caché de archivos de audio
        ColaCleanup     cleanup_queue;      ///< Cola de limpieza de sonidos a desinicializar

        /**
        * @brief Constructor de Impl
        *  El constructor del Impl hace el cast de los punteros opacos
        * @param context (ma_context*) Contexto de audio de miniaudio
        * @param devInfo (ma_device_info*) Información del dispositivo
        */
        Impl(void* context, const void* devInfo);

        /**
        * @brief Marca como finalizado el sonido que acaba de reproducirse (finaliza)
        * @details Invocado directamente desde el hilo de procesamiento en tiempo real de Miniaudio (evitar logica pesada)
        * @param userData Datos del usuario, típicamente el puntero a esta instancia.
        * @param sound Puntero al sonido que terminó de reproducirse.
        */
        static void endCallback(void* userData, ma_sound* sound);
    };


    // Implementación de métodos PIMPL ------------------------------------------------------

    AudioPlaybackModule::Impl::Impl(void* context, const void* devInfo) {
        ctx = static_cast<ma_context*>(context);
        device_info = *static_cast<const ma_device_info*>(devInfo);
    }

    void AudioPlaybackModule::Impl::endCallback(void* userData, ma_sound* sound) {
        auto* self = reinterpret_cast<AudioPlaybackModule*>(userData);

        // Busca el sonido en la lista de sonidos reproduciéndose para eliminarlo
        self->sendToCleanup(sound);
        self->cleanup_cv_.notify_one();
        //self->cleanup();  // método directo (no diferido)
    }


    // General ------------------------------------------------------------------------------

    AudioPlaybackModule::AudioPlaybackModule(void* ctx, void* const device_info, std::string const& audioFolder) :
        pimpl_(std::make_unique<Impl>(ctx, device_info)),
        initialized_(false),
        audioFolder_(audioFolder),
        keep_alive_seconds_(10)
    {

    }

    AudioPlaybackModule::~AudioPlaybackModule() {
        stop();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool AudioPlaybackModule::init() {
        if (initialized_)
            return true;

        // Inicializar el dispositivo de reproducción
        ma_engine_config config = ma_engine_config_init();
        config.pContext = pimpl_->ctx;
        config.pPlaybackDeviceID = &pimpl_->device_info.id;
        if (ma_engine_init(&config, &pimpl_->engine) != MA_SUCCESS) {
            SYS_ERROR("AudioPlaybackModule","Engine init failed");
            return false;
        }

        // Activar running para los hilos
        running_ = true;

        // Inicializar el hilo de limpieza diferida
        SYS_INFO("PlaybackModule","Starting audio cleanup thread...");
        cleanup_thread_ = std::thread(&AudioPlaybackModule::TCleanup, this);

        // Iniciar el hilo del tiempo de vida de la caché de audios
        SYS_INFO("PlaybackModule","Starting audio caché cleanup on timeout thread...");
        cachereaper_thread_ = std::thread(&AudioPlaybackModule::TCacheReaperWorker, this);

        initialized_ = true;
        return true;
    }

    void AudioPlaybackModule::stop() {
        if (!initialized_)
            return;

        SYS_INFO("PlaybackModule", "Stopping PlaybackModule...");

        running_ = false;

        // Apagar el hilo de limpieza primero
        if (cleanup_thread_.joinable()) {
            SYS_INFO("PlaybackModule","Closing cleanup thread...");
            cleanup_cv_.notify_all(); // Despertar al hilo para que finalice
            cleanup_thread_.join();
        }

        // Despertar y unir hilo del Reaper de caché
        if (cachereaper_thread_.joinable()) {
            SYS_INFO("PlaybackModule","Closing sounds reaper thread...");
            cachereaper_cv_.notify_all();
            cachereaper_thread_.join();
        }

        // Limpiar instancias de sonidos activos
        {
            std::lock_guard<std::mutex> lock(playing_sounds_mtx_);
            SYS_INFO("PlaybackModule","Unitializing active sounds...");
            for (auto& [id, snd] : pimpl_->playing_sounds){
                ma_sound_uninit(&snd->sound);
                if (snd->isBuffer)
                    ma_audio_buffer_uninit(&snd->buffer);
            }
            pimpl_->playing_sounds.clear();
        }
        cleanupSounds();    // Asegurar limpieza

        // Limpiar cache de audios
        {
            std::lock_guard<std::mutex> lock(sounds_cache_mtx_);
            SYS_INFO("PlaybackModule","Cleaning audio cache...");
            for (auto& [wavname, snd] : pimpl_->sounds_cache)
                ma_sound_uninit(&snd->sound);
            pimpl_->sounds_cache.clear();
        }

        // Desinicializar el sistema
        SYS_INFO("PlaybackModule","Unitializing audio engine...");
        ma_engine_uninit(&pimpl_->engine);

        initialized_ = false;
        return;
    }


    // Acciones -----------------------------------------------------------------------------

    void AudioPlaybackModule::playAudio(
        std::string const&  filepath,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        if (!initialized_)
            return;

        // Asegurar precarga de audio en caché (devuelve true si ya estaba precargado)
        if (!preloadAudioFile(filepath)) {
            SYS_WARN("AudioPlaybackModule", "Could not preload: " + filepath);
            return;
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Bloquear caché para leer la muestra precargada de forma segura
        {
            std::lock_guard<std::mutex> cacheLock(sounds_cache_mtx_);
            auto it = pimpl_->sounds_cache.find(filepath);
            if (it == pimpl_->sounds_cache.end()) return;

            ma_result res = ma_sound_init_copy(
                &pimpl_->engine,
                &it->second->sound,
                0,
                nullptr,
                &inst->sound);

            if (res != MA_SUCCESS) {
                SYS_WARN("AudioPlaybackModule", "ma_sound_init_copy error");
                return;
            }
        }

        // Establecer parámetros de la reproducción
        ma_sound_set_volume(&inst->sound, static_cast<float>(volume)/100);  // (va de 0.0 a 1.0)
        ma_sound_set_pitch(&inst->sound, pitch);
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE : MA_FALSE);

        // Vincular el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        // Obtener el nombre del archivo para el mapa
        std::filesystem::path path = std::filesystem::absolute(filepath);
        std::string filename = path.stem().string();

        // Guardar parámetros en la instancia de sonido guardado
        inst->loopMode  = loop;
        inst->forceStop = forceStop;
        inst->name      = filename;

        // Guardar en el mapa y reproducir
        {
            // Guardar la instancia de sonido en la lista de sonidos reproduciéndose
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[filename] = std::move(inst);

            // Comenzar a reproducir (tomo directamente el sonido de la lista de playing_sounds)
            SYS_INFO("PlaybackModule","'" + filename + "': init playing...");
            ma_sound_start(&pimpl_->playing_sounds[filename]->sound);
        }
    }

    void AudioPlaybackModule::playFromFolder(
        std::string const&  filename,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch)
    {
        if (audioFolder_.empty()) {
            SYS_WARN("AudioPlaybackModule", "playFromFolder: este playback no tiene carpeta de audios configurada");
            return;
        }

        // Construye la ruta completa a partir de la carpeta configurada y el nombre del archivo
        std::filesystem::path fullPath = std::filesystem::path(audioFolder_) / filename;

        playAudio(fullPath.string(), volume, loop, forceStop, pitch);
    }

    void AudioPlaybackModule::stopAudio(std::string const& audioName, bool force) {

        // Info
        std::string stopType = (force) ? "forced" : "soft";
        SYS_INFO("PlaybackModule", "Commanded " + stopType + " stop to '" + audioName + "'");

        // Buscar el sonido reproduciéndose
        std::unique_lock<std::mutex> lock(playing_sounds_mtx_);
        auto it = pimpl_->playing_sounds.find(audioName);
        if (it == pimpl_->playing_sounds.end()) {
            SYS_WARN("PlaybackModule","Error stopping audio: '" + audioName + "' not playing");
            return;
        }

        // Desbloquear mutex para evitar bloqueos de las funciones de después (sendToCleanup)
        lock.unlock();

        // Obtener la SoundInstance
        std::unique_ptr<AudioPlaybackModule::Impl::SoundInstance>& audioInstance = it->second;

        // Si LoopMode activo, desactivar loop y dejar que acabe
        if(audioInstance->loopMode)
            ma_sound_set_looping(&audioInstance->sound, MA_FALSE);

        // Si forceStop de la instancia o param force activo, cortar inmediatamente
        else if(audioInstance->forceStop || force) {
            ma_sound_stop(&audioInstance->sound);
            sendToCleanup(&audioInstance->sound);
            SYS_INFO("PlaybackModule","'" + audioName + "': stop forced");
            cleanup_cv_.notify_one();
        }

        // si !forcestop, no hacer nada y dejar acabar el audio cuando termine
    }

    void AudioPlaybackModule::setVolume(std::string const& audioName, float volume) {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        float volume_normalized = static_cast<float>(volume)/100;

        auto it = pimpl_->playing_sounds.find(audioName);
        if (it != pimpl_->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "' volume changed: " + std::to_string((int)volume) + "/100");
            ma_sound_set_volume(&it->second->sound, volume_normalized);
        }
        else SYS_WARN("PlaybackModule", "Change volume fail: '" + audioName + "' not found");
    }

    void AudioPlaybackModule::setPitch(std::string const& audioName, float pitch) {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        auto it = pimpl_->playing_sounds.find(audioName);
        if (it != pimpl_->playing_sounds.end()) {
            SYS_INFO("PlaybackModule","'" + audioName + "': pitch changed:" + std::to_string(pitch));
            ma_sound_set_pitch(&it->second->sound, pitch);
        }
        else SYS_WARN("PlaybackModule", "Change pitch fail: '" + audioName + "' not found");
    }

// MORSE --------------------------------------------------------------------------------

    bool AudioPlaybackModule::playMorse(
        std::string const& texto,
        std::string const& audioName,
        float              frequencyHz,
        unsigned int       puntoMs,
        unsigned int       rayaMs,
        unsigned int       espacioEntreSimbolos,
        unsigned int       espacioEntreLetras,
        unsigned int       sampleRate,
        unsigned int       espacioEntreMorse,
        unsigned short     volume,
        bool               loop
    ){
        if (!initialized_)
            return false;

        // Generar el audio a partir del texto
        std::vector<float> audio = generateMorseAudio(texto, frequencyHz, puntoMs, rayaMs, espacioEntreSimbolos, espacioEntreLetras, sampleRate, espacioEntreMorse);
        if(audio.empty()) {
            SYS_WARN("AudioPlaybackModule", "playMorse: audio vacío para '" + texto + "'"); 
            return false; 
        }

        // Crear la instancia de sonido
        auto inst = std::make_unique<Impl::SoundInstance>();

        // Describir el audio generado (formato, canales, tamaño, puntero a los datos)
        ma_audio_buffer_config config = ma_audio_buffer_config_init(
            ma_format_f32,
            1, 
            audio.size(), 
            audio.data(), 
            nullptr
        );
        config.sampleRate = sampleRate; 

        // Copiar los datos generados a un buffer propio de miniaudio
        if (ma_audio_buffer_init_copy(&config, &inst->buffer) !=MA_SUCCESS){
            SYS_WARN("AudioPlaybackModule", "playMorse: fallo al crear el buffer de audio");
            return false; 
        }
        inst->isBuffer = true; 

        // Envolver el buffer como un sonido reproducible por el motor
        if (ma_sound_init_from_data_source(&pimpl_->engine, &inst->buffer, 0, nullptr, &inst->sound) !=MA_SUCCESS){
            ma_audio_buffer_uninit(&inst->buffer);
            SYS_WARN("AudioPlaybackModule", "playMorse: fallo al inicializar el sonido");
            return false; 
        }

        //Establecer parámetros de la reproducción
        ma_sound_set_volume(&inst->sound, static_cast<float>(volume) / 100.0f);
        ma_sound_set_looping(&inst->sound, (loop) ? MA_TRUE :MA_FALSE);

        // Vincluar el fin de la reproducción al endCallback
        ma_sound_set_end_callback(&inst->sound, pimpl_->endCallback, this);

        //Guardar parámetros en la instancia de sonido
        inst->loopMode = loop;
        inst->name     = audioName; 

        // Guardar en el mapa y reproducir
        {
            std::lock_guard<std::mutex> soundsLock(playing_sounds_mtx_);
            pimpl_->playing_sounds[audioName] = std::move(inst);

            SYS_INFO("PlaybackModule","'" + audioName + "': init playing morse...");
            ma_sound_start(&pimpl_->playing_sounds[audioName]->sound);
        }

        return true;
    }


    // Datos del módulo ---------------------------------------------------------------------

    bool AudioPlaybackModule::isPlaying(std::string const& name) const {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        auto it = pimpl_->playing_sounds.find(name);
        if (it == pimpl_->playing_sounds.end())
            return false;

        return ma_sound_is_playing(&it->second->sound);
    }

    // #TODO CAMBIAR: de alguna manera, obtener un dispositivo libre que no esté reproduciendo (el siguiente, por ejemplo)
    bool AudioPlaybackModule::isBusy() const {
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        // Devuelve si hay algún sonido en la lista de playing_sounds
        return !pimpl_->playing_sounds.empty();
    }

    std::string AudioPlaybackModule::deviceName() const {
        return pimpl_->device_info.name;
    }


    // Caché --------------------------------------------------------------------------------

    bool AudioPlaybackModule::preloadAudioFile(const std::string& filepath) {
        if (!initialized_)
            return false;

        std::unique_lock<std::mutex> lock(sounds_cache_mtx_);

        // Comprobar si ya está precargado
        if (pimpl_->sounds_cache.count(filepath))
            return true;

        // Desbloquear para la carga
        lock.unlock();

        // Crear una instancia de la estructura de la caché de audios
        auto inst = std::make_unique<Impl::AudioCacheInstance>();

        // Inicializar el audio de la instancia a partir del archivo
        if (ma_sound_init_from_file(
            &pimpl_->engine,
            filepath.c_str(),
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            &inst->sound) != MA_SUCCESS)
        {
            SYS_WARN("AudioPlaybackModule", "Failed to load audio file: " + filepath);
            return false;
        }

        // Guardar la marca de tiempo
        std::unique_lock<std::mutex> reaperLock(cachereaper_mtx_);
        inst->time = std::chrono::steady_clock::now();
        reaperLock.unlock();

        // Bloquear de nuevo para el acceso a la caché
        lock.lock();

        // Guardar el audio generado en la caché
        pimpl_->sounds_cache[filepath] = std::move(inst);

        return true;
    }


    // Limpieza -----------------------------------------------------------------------------

    void AudioPlaybackModule::TCleanup() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        // Bucle de mientras haya sonidos pendientes de desinicializar
        while ( running_ || !pimpl_->cleanup_queue.empty()) {

            std::unique_lock<std::mutex> lock(cleanup_mtx_);
            // Esperar hasta que haya un sonido en cola o se ordene apagar
            cleanup_cv_.wait(lock, [this]() {
                return !pimpl_->cleanup_queue.empty() || !running_;
            });

            // Si se ordenó apagar y ya no quedan sonidos por limpiar, salir del bucle
            if (!running_ && pimpl_->cleanup_queue.empty())
                break;

            // Extraer el sonido de la cola
            if (!pimpl_->cleanup_queue.empty()) {
                instanceToClean = std::move(pimpl_->cleanup_queue.front());
                pimpl_->cleanup_queue.pop();
            }

            // Libera el lock de aquí en adelante
            lock.unlock();

            // Si tenemos la instancia extraída:
            if (instanceToClean) {
                // Desinicializar el recurso de miniaudio de forma segura en este hilo
                ma_sound_uninit(&instanceToClean->sound);

                // Si venía de un buffer generado (ej. morse), liberar también ese buffer
                if (instanceToClean->isBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);

                // Al llegar al final de este bloque, instanceToClean se destruye automáticamente
                // liberando la memoria de SoundInstance sin tocar playing_sounds (ya no estaba ahí).
            }
        }
    }

    void AudioPlaybackModule::sendToCleanup(void* sound) {
        ma_sound* maSound = static_cast<ma_sound*>(sound);

        // Proteger la lista PlayingSounds
        std::lock_guard<std::mutex> lock(playing_sounds_mtx_);

        // Buscar la clave del sonido que ha terminado
        for (auto it = pimpl_->playing_sounds.begin(); it != pimpl_->playing_sounds.end(); ++it) {
            if (&it->second->sound == maSound) {

                // Notificar que el audio ha terminado
                SYS_INFO("PlaybackModule", "'" + it->second->name + "': audio finished");

                // Extraer el nodo del mapa sin destruir la memoria del unique_ptr
                auto node = pimpl_->playing_sounds.extract(it);

                // Mover la propiedad de la instancia a la cola de limpieza
                {
                    std::lock_guard<std::mutex> cleanLock(cleanup_mtx_);
                    pimpl_->cleanup_queue.push(std::move(node.mapped()));
                }

                // Avisar al hilo de limpieza que hay trabajo pendiente
                cleanup_cv_.notify_one();
                break;
            }
        }
    }

    void AudioPlaybackModule::cleanupSounds() {

        std::unique_ptr<Impl::SoundInstance> instanceToClean = nullptr;

        std::lock_guard<std::mutex> lock(cleanup_mtx_);
        while (!pimpl_->cleanup_queue.empty()) {
            instanceToClean = std::move(pimpl_->cleanup_queue.front());
            pimpl_->cleanup_queue.pop();
            if (instanceToClean){
                ma_sound_uninit(&instanceToClean->sound);
                if (instanceToClean->isBuffer)
                    ma_audio_buffer_uninit(&instanceToClean->buffer);
            }
        }
    }

// Limpieza de caché de audios ----------------------------------------------------------

    void AudioPlaybackModule::TCacheReaperWorker() {

        // Tiempo de comprobación, cada tiempoVida/10 con mínimo de 1seg
        std::chrono::duration<long> poll_interval = std::max(std::chrono::seconds(1), keep_alive_seconds_ / 10);

        // Mutex del reaper (se desbloquea en el wait_for)
        std::unique_lock<std::mutex> lock(cachereaper_mtx_);

        while (running_) {

            // Espera hasta: cierre, haya algo que vigilar
            cachereaper_cv_.wait_for(lock, poll_interval, [this] {
                return !running_;
            });

            // Salir si no está activo el módulo (se está cerrando)
            if (!running_) break;

            /* #TODO */

            // Bloquear mutex de la lista de sonidos
            std::unique_lock<std::mutex> soundsLock(sounds_cache_mtx_);

            // Comprobar si hay algo que comprobar (xd)
            if (pimpl_->sounds_cache.empty()) continue;

            // Compara el tiempo de los modelos con el actual
            auto now = std::chrono::steady_clock::now();

            // Borrar si se ha superado el tiempo y no se está usando (así para evitar segmentation-fault)
            bool isCurrentlyPlaying = false;
            for (auto it = pimpl_->sounds_cache.begin(); it != pimpl_->sounds_cache.end(); ) {
                if (now - it->second->time >= keep_alive_seconds_) {

                    // No liberar de la caché si hay algún sonido sonando
                    isCurrentlyPlaying = false;
                    {
                        std::unique_lock<std::mutex> pLock(playing_sounds_mtx_);
                        // Comprobar si la ruta coincide con alguno de los activos
                        std::filesystem::path p(it->first);
                        std::string stem = p.stem().string();
                        if (pimpl_->playing_sounds.count(stem)) {
                            isCurrentlyPlaying = true;
                        }
                    }

                    if (!isCurrentlyPlaying) {
                        SYS_INFO("PlaybackModule", "Freeing cached sound: " + it->first);
                        ma_sound_uninit(&it->second->sound);
                        it = pimpl_->sounds_cache.erase(it);
                    } else {
                        // Renovar expiración para reintentar en el siguiente ciclo
                        it->second->time = now;
                        ++it;
                    }
                } else {
                    ++it;
                }
            }

            // Desbloquear el mutex de la lista de sonidos
            soundsLock.unlock();
        }
    }


// MORSE --------------------------------------------------------------------------------

    // Se puede hacer función libre
     std::vector<float> AudioPlaybackModule::generateMorseAudio(
        std::string const& texto,
        float              frequencyHz,
        unsigned int       puntoMs,
        unsigned int       rayaMs,
        unsigned int       espacioEntreSimbolos,
        unsigned int       espacioEntreLetras,
        unsigned int       sampleRate,
        unsigned int       espacioEntreMorse
    ){
        std::vector<float> audio;

        // Recorrer el texto letra por letra
        for (size_t c=0; c < texto.size(); ++c){

            int letra = std::toupper(static_cast<unsigned char>(texto[c]));

            // Espacio: separación entre palabras
            if(letra == ' '){
                size_t silentSamples = sampleRate * espacioEntreMorse / 1000;
                for (size_t i = 0; i < silentSamples; ++i)
                    audio.push_back(0.0f);
                continue;
            }

            // Letra no soportada por el diccionario: se ignora
            if (MORSE_DICT.find(letra) == MORSE_DICT.end())
                continue;

            std::string code = MORSE_DICT.at(letra);

            // Generar el pitido de cada letra: puntos, rayas y espacio entre símbolos
            for (size_t s = 0; s < code.size(); ++s) {

                // Punto o raya, según el símbolo
                unsigned int toneMs = (code[s] == '-') ? rayaMs : puntoMs;
                size_t toneSamples = sampleRate * toneMs / 1000;

                // Generar el tono (onda senoidal) y guardarlo en audio
                for (size_t i = 0; i < toneSamples; ++i) {
                    float t = static_cast<float>(i) / sampleRate;
                    audio.push_back(sin(2.0f * 3.14159265f * frequencyHz * t));
                }

                // Silencio entre símbolos de la misma letra
                if (s + 1 < code.size()) {
                    size_t gapSamples = sampleRate * espacioEntreSimbolos / 1000;
                    for (size_t i = 0; i < gapSamples; ++i)
                        audio.push_back(0.0f);
                }
            }

            // Espacio entre letras de la misma palabra
            if (c + 1 < texto.size() && texto[c + 1] != ' ') {
                size_t gapSamples = sampleRate * espacioEntreLetras / 1000;
                for (size_t i = 0; i < gapSamples; ++i)
                    audio.push_back(0.0f);
            }
        }

        return audio;
    }


#endif
