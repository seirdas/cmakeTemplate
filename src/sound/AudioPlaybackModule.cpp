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

    std::lock_guard<std::mutex> lock(mutex_);

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
    
    std::lock_guard<std::mutex> lock(mutex_);

    // Limpiar instancias activas
    for (auto& [id, inst] : sounds_) {
        ma_sound_uninit(&inst->sound);
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

    std::lock_guard<std::mutex> lock(mutex_);

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

SoundID AudioPlaybackModule::play(const std::string& filepath,
                                  float volume,
                                  float pitch,
                                  LoopMode loop)
{
    cleanupFinished();

    if (!running_)
        return 0;

    std::lock_guard<std::mutex> lock(mutex_);


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

    ma_sound_set_volume(&inst->sound, volume);
    ma_sound_set_pitch(&inst->sound, pitch);

    if (loop == LoopMode::LOOP)
        ma_sound_set_looping(&inst->sound, MA_TRUE);

    inst->loopMode = loop;

    SoundID id = idCounter_++;

    ma_sound_set_end_callback(&inst->sound, endCallback, this);
    ma_sound_start(&inst->sound);

    sounds_[id] = std::move(inst);

    return id;
}

void AudioPlaybackModule::stopSound(SoundID id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    auto& s = it->second;

    if (s->loopMode == LoopMode::NONSTOP)
        ma_sound_set_looping(&s->sound, MA_FALSE);
    else
        ma_sound_stop(&s->sound);
}

void AudioPlaybackModule::setVolume(SoundID id, float volume)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    ma_sound_set_volume(&it->second->sound, volume);
}

void AudioPlaybackModule::setPitch(SoundID id, float pitch)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return;

    ma_sound_set_pitch(&it->second->sound, pitch);
}

bool AudioPlaybackModule::isPlaying(SoundID id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sounds_.find(id);
    if (it == sounds_.end())
        return false;

    return ma_sound_is_playing(&it->second->sound);
}


std::string AudioPlaybackModule::deviceName() const {
    return device_info_.name;
}

const ma_device_id AudioPlaybackModule::getDeviceID() const {
    return device_info_.id;
}

void AudioPlaybackModule::endCallback(void* userData, ma_sound* sound)
{
    auto* self = reinterpret_cast<AudioPlaybackModule*>(userData);
    std::lock_guard<std::mutex> lock(self->mutex_);

    for (auto& [id, s] : self->sounds_)
    {
        if (&s->sound == sound)
        {
            s->finished.store(true, std::memory_order_relaxed);
            break;
        }
    }
}


void AudioPlaybackModule::cleanupFinished()
{
    // PRECONDITION: mutex_ already locked
    for (auto it = sounds_.begin(); it != sounds_.end(); ) {
        if (it->second->finished)
        {
            ma_sound_uninit(&it->second->sound);
            it = sounds_.erase(it);
        }
        else ++it;
    }
}
