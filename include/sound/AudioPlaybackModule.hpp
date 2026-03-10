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
    std::atomic<bool> finished{false};
};

class AudioPlaybackModule
{
public:

    AudioPlaybackModule(ma_context* ctx, ma_device_info const& device_info);
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

    std::string deviceName() const;


private:

    const ma_device_id getDeviceID() const;

    static void endCallback(void* userData, ma_sound* sound);

    void cleanupFinished();

private:

    ma_context* context_;
    ma_device_info device_info_;

    ma_engine engine_;

    std::unordered_map<SoundID, std::unique_ptr<SoundInstance>> sounds_;
    std::unordered_map<std::string, std::unique_ptr<ma_sound>> cache_;

    std::mutex mutex_;

    std::atomic<SoundID> idCounter_{1};
    bool running_ = false;
};
