
#include <miniaudio.h>
#include <iostream>

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>


class SoundMgr{

public:
    SoundMgr(){

    }

    ~SoundMgr(){
        std::cout << "[SoundMgr] Closing sound engine..." << std::endl;
        ma_engine_uninit(&engine_);
    }

    bool init(){
        std::cout << "[SoundMgr] Initializating sound engine..." << std::endl;
        ma_result res = ma_engine_init(NULL, &engine_);
        return (res == MA_SUCCESS) ? true : false;
    }

    void test(){
        ma_engine_play_sound(&engine_, "DefaultDance.mp3", NULL);
    }

    void test2(){
        ma_engine_play_sound(&engine_, "chinos.mp3", NULL);
    }

    void record(){
        int a = 0;
        a = 2+a;

        std::cout << "[SoundMgr] Recording 5 seconds to output.wav..." << std::endl;

        const ma_uint32 sampleRate = 44100;
        const ma_uint32 channels = 2;
        const ma_uint32 secondsToRecord = 10;

        struct RecordingContext {
            ma_encoder encoder;
            std::atomic<uint64_t> framesWritten;
            uint64_t maxFrames;
        };

        RecordingContext ctx{};
        ctx.framesWritten = 0;
        ctx.maxFrames = (uint64_t)sampleRate * secondsToRecord;

        // --- Encoder WAV ---
        ma_encoder_config encoderConfig =
            ma_encoder_config_init(ma_encoding_format_wav,
                                ma_format_s16,
                                channels,
                                sampleRate);

        if (ma_encoder_init_file("output.wav", &encoderConfig, &ctx.encoder) != MA_SUCCESS) {
            std::cout << "[SoundMgr] Failed to initialize encoder." << std::endl;
            return;
        }

        // --- Callback de captura ---
        auto dataCallback = [](ma_device* device, void* output, const void* input, ma_uint32 frameCount)
        {
            RecordingContext* ctx = (RecordingContext*)device->pUserData;

            if (input == nullptr)
                return;

            uint64_t framesRemaining = ctx->maxFrames - ctx->framesWritten.load();
            uint64_t framesToWrite = frameCount;

            if (framesToWrite > framesRemaining)
                framesToWrite = framesRemaining;

            if (framesToWrite > 0) {
                ma_uint64 actuallyWritten = 0;

                ma_result res = ma_encoder_write_pcm_frames(
                    &ctx->encoder,
                    input,
                    framesToWrite,
                    &actuallyWritten
                );

                if (res == MA_SUCCESS) {
                    ctx->framesWritten += actuallyWritten;
                }
            }

            (void)output;
        };

        // --- Configuración dispositivo ---
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
        deviceConfig.capture.format = ma_format_s16;
        deviceConfig.capture.channels = channels;
        deviceConfig.sampleRate = sampleRate;
        deviceConfig.dataCallback = dataCallback;
        deviceConfig.pUserData = &ctx;

        ma_device device;
        if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
            std::cout << "[SoundMgr] Failed to initialize capture device." << std::endl;
            ma_encoder_uninit(&ctx.encoder);
            return;
        }

        if (ma_device_start(&device) != MA_SUCCESS) {
            std::cout << "[SoundMgr] Failed to start capture device." << std::endl;
            ma_device_uninit(&device);
            ma_encoder_uninit(&ctx.encoder);
            return;
        }

        // Esperar hasta terminar
        unsigned int count=0;
        while (ctx.framesWritten < ctx.maxFrames) {
            std::cout << count << ": frameswritten" << ctx.framesWritten;
            std::cout << ", maxFrames: " << ctx.maxFrames << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            count++;
        }

        ma_device_stop(&device);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        ma_device_uninit(&device);
        ma_encoder_uninit(&ctx.encoder);

        std::cout << "[SoundMgr] Recording finished." << std::endl;
    }



private:

    /************ Variables ********************************************************/

    ma_engine engine_;

};