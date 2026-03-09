#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <miniaudio.h>

enum class LoopMode
{
    NONE,
    LOOP,
    NONSTOP
};

using SoundID = uint64_t;

struct SoundInstance
{
    ma_sound sound;
    LoopMode loopMode = LoopMode::NONE;
    bool finished = false;
};

class AudioPlaybackModule
{
public:

    AudioPlaybackModule(ma_context* ctx,
                        const ma_device_id& deviceID,
                        const std::string& deviceName);

    ~AudioPlaybackModule();

    bool start();
    void stop();

    bool preload(const std::string& filepath);

    SoundID play(const std::string& filepath,
                 float volume = 1.0f,
                 float pitch = 1.0f,
                 LoopMode loop = LoopMode::NONE);

    void stopSound(SoundID id);

    void setVolume(SoundID id, float volume);
    void setPitch(SoundID id, float pitch);

    bool isPlaying(SoundID id);

    const std::string& deviceName() const;

private:

    static void endCallback(void* userData, ma_sound* sound);

    void cleanupFinished();

private:

    ma_context* context_;
    ma_device_id device_id_;
    std::string device_name_;

    ma_engine engine_;

    std::unordered_map<SoundID, std::unique_ptr<SoundInstance>> sounds_;
    std::unordered_map<std::string, ma_sound*> cache_;

    std::mutex mutex_;

    std::atomic<SoundID> idCounter_{1};

    bool running_ = false;
};