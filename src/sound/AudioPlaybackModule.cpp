#include "sound/AudioPlaybackModule.hpp"
#include "system/SystemMgr.hpp"

AudioPlaybackModule::AudioPlaybackModule(ma_context* ctx, ma_device_info const& device_info)
    :
    context_(ctx), device_info_(device_info)
{

}

AudioPlaybackModule::~AudioPlaybackModule()
{
    stop();
}

bool AudioPlaybackModule::start()
{
    if (running_)
        return true;

    std::lock_guard<std::mutex> lock(sounds_mtx_);

    // Inicializar el dispositivo de reproducción
    ma_engine_config config = ma_engine_config_init();
    config.pContext = context_;
    config.pPlaybackDeviceID = &device_info_.id; 
    if (ma_engine_init(&config, &engine_) != MA_SUCCESS) {
        SYS_ERROR("AudioPlaybackModule","Engine init failed");
        return false;
    }

    running_ = true;
    return true;
}

void AudioPlaybackModule::stop()
{
    if (!running_)
        return;
    
    std::lock_guard<std::mutex> lock(sounds_mtx_);

    // Limpiar instancias activas
    for (auto& [id, inst] : sounds_) {
        ma_sound_uninit(&inst->sound);

        if(inst->isbuffer){
            ma_audio_buffer_uninit(&inst->buffer);
        }
    }
    sounds_.clear();

    // Limpiar cache
    for (auto& [path, snd] : cache_) {
        ma_sound_uninit(snd.get());
    }
    cache_.clear();

    // Desinicializar el sistema
    ma_engine_uninit(&engine_);
    running_ = false;
}

bool AudioPlaybackModule::preload(const std::string& filepath)
{
    if (!running_)
        return false;

    std::lock_guard<std::mutex> lock(sounds_mtx_);

    // Comprobar si ya está precargado
    if (cache_.count(filepath))
        return true;

    
    auto snd = std::make_unique<ma_sound>();

    if (ma_sound_init_from_file(
            &engine_,
            filepath.c_str(),
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            snd.get()) != MA_SUCCESS)
    {
        return false;
    }

    cache_[filepath] = std::move(snd);

    return true;
}

unsigned long long AudioPlaybackModule::play(const std::string& filepath,
    unsigned short volume,
    bool loop,
    bool forceStop,
    unsigned short pitch)
{
    cleanupFinished();

    if (!running_)
        return 0;

    std::lock_guard<std::mutex> lock(sounds_mtx_);

    auto inst = std::make_unique<SoundInstance>();

    ma_result res;

    if (cache_.count(filepath))
    {
        res = ma_sound_init_copy(
            &engine_,
            cache_[filepath].get(),
            0,
            nullptr,
            &inst->sound);
    }
    else
    {
        res = ma_sound_init_from_file(
            &engine_,
            filepath.c_str(),
            0,
            nullptr,
            nullptr,
            &inst->sound);
    }

    if (res != MA_SUCCESS)
        return 0;

    ma_sound_set_volume(&inst->sound, volume/100);
    ma_sound_set_pitch(&inst->sound, pitch);

    if (loop)
        ma_sound_set_looping(&inst->sound, MA_TRUE);

    inst->loopMode = loop;

    unsigned long long id = idCounter_++;

    ma_sound_set_end_callback(&inst->sound, endCallback, this);
    ma_sound_start(&inst->sound);

    sounds_[id] = std::move(inst);

    return id;
}

void AudioPlaybackModule::stopSound(unsigned long long id)
{
    std::lock_guard<std::mutex> lock(sounds_mtx_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    auto& s = it->second;

    if (s->loopMode)
        ma_sound_set_looping(&s->sound, MA_FALSE);
    else
        ma_sound_stop(&s->sound);
}

void AudioPlaybackModule::setVolume(unsigned long long id, float volume)
{
    std::lock_guard<std::mutex> lock(sounds_mtx_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    ma_sound_set_volume(&it->second->sound, volume);
}

void AudioPlaybackModule::setPitch(unsigned long long id, float pitch)
{
    std::lock_guard<std::mutex> lock(sounds_mtx_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    ma_sound_set_pitch(&it->second->sound, pitch);
}

unsigned long long AudioPlaybackModule::playBuffer( std::vector<float> const& audio,
        unsigned int sampleRate,
        unsigned short volume,
        bool loop,
        bool forceStop,
        unsigned short pitch)
    {
        cleanupFinished();

        if(!running_ || audio.empty())
        return 0; 

        std::lock_guard<std::mutex> lock(sounds_mtx_);

        auto inst = std::make_unique<SoundInstance>(); //se crea un objeto vacío 

        // creamos la configuracion del audio
        ma_audio_buffer_config config = ma_audio_buffer_config_init(
            ma_format_f32, 
            1,                  //mono
            audio.size(), 
            audio.data(), 
            nullptr
        );
        config.sampleRate = sampleRate; 

        // comprueba si la copia salió bien
        if (ma_audio_buffer_init_copy(&config, &inst->buffer) != MA_SUCCESS)
            return 0; 
            inst->isbuffer = true; // si ha salido se activa el buffer

        //coge el buffer con el audio morse
        if (ma_sound_init_from_data_source(&engine_, &inst->buffer, 0, nullptr, &inst->sound) !=MA_SUCCESS)
        {
            ma_audio_buffer_uninit(&inst->buffer); //si falla vaciamos el buffer
            return 0; 
        }

        // mandamos el volumen y el pitch al sonido que queremos
        ma_sound_set_volume(&inst->sound, volume / 100.0f); 
        ma_sound_set_pitch(&inst->sound, pitch); 

        // si el parametro loop es true
        if(loop)
        ma_sound_set_looping(&inst->sound, MA_TRUE); // se dice a miniaudio que cuando acabe vuelva a empezar

        inst->loopMode = loop; // guarda si está en bucle o no

        unsigned long long id = idCounter_++; //genera un ID para cada sonido

        ma_sound_set_end_callback(&inst->sound, endCallback, this); //cuando acabe llama a endCallback
        
        //reproduce el sonido
        ma_sound_start(&inst->sound);

        //guarda en la posición id del mapa, el SoundInstance
        sounds_[id]=std::move(inst);

        return id; 
    }

bool AudioPlaybackModule::isPlaying(unsigned long long id) const
{
    std::lock_guard<std::mutex> lock(sounds_mtx_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return false;

    return ma_sound_is_playing(&it->second->sound);
}

// #TODO CAMBIAR: de alguna manera, obtener un dispositivo libre que no esté reproduciendo (el siguiente, por ejemplo)
bool AudioPlaybackModule::isBusy() const {
    std::lock_guard<std::mutex> lock(sounds_mtx_); 
    for (auto& [id, inst] : sounds_) //recorre el ID del sonido y si ha terminado de sonar o no de todos los sonidos
        if (!inst->finished) // si no encuentra ningun playback reproduciendose es que estan todos libres 
            return true; // si recorre todos y hay alguno sonando devuelve true 
    return false; // si recorre todos y no hay ninguno sonando devuelve false
}

std::string AudioPlaybackModule::deviceName() const {
    return device_info_.name;
}

ma_device_id AudioPlaybackModule::getDeviceID() const {
    return device_info_.id;
}

void AudioPlaybackModule::endCallback(void* userData, ma_sound* sound)
{
    auto* self = reinterpret_cast<AudioPlaybackModule*>(userData);
    std::lock_guard<std::mutex> lock(self->sounds_mtx_);

    for (auto& [id, s] : self->sounds_)
    {
        if (&s->sound == sound)
        {
            s->finished = true;
            break;
        }
    }
}

void AudioPlaybackModule::cleanupFinished()
{
    // PRECONDITION: mutex_ already locked
    for (auto it = sounds_.begin(); it != sounds_.end(); ) {

        // Solo actuamos sobre los sonidos que ya han terminado de sonar
        if (it->second->finished)
        {
            // Libera los recursos de miniaudio del ma_sound (siempre hace falta)
            ma_sound_uninit(&it->second->sound);

            // Si este sonido venía de un buffer en memoria (morse), liberar también ese buffer
            if (it->second->isbuffer) {
                ma_audio_buffer_uninit(&it->second->buffer);
            }

            // Borra la entrada del mapa; erase() devuelve el iterador al siguiente elemento
            it = sounds_.erase(it);
        }
        else ++it; // si no ha terminado, simplemente avanzamos al siguiente
    }
}
