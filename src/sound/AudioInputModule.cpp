
#include "sound/AudioInputModule.hpp"
#include <iostream>

// General ------------------------------------------------------------------------------

AudioInputModule::AudioInputModule(ma_context* ctx)
    : snd_ctx_(ctx)
{
    rec_ctx_.recording = false;
};

AudioInputModule::~AudioInputModule() {

    // Parar grabaciones activas
    if(isRecording()){
        std::cout << "[AudioInputModule]    Stopping running recorders..." << std::endl;
        StopRec();
    } 
};


// Grabación ----------------------------------------------------------------------------

bool AudioInputModule::StartRec(std::string const& filename) {

    if (rec_ctx_.recording)
        return false;

    std::cout << "[AudioInputModule]    Start recording to " << filename << ".wav ..." << std::endl;

    rec_ctx_.filename = filename+".wav";
    rec_ctx_.framesWritten.store(0);
    rec_ctx_.maxFrames = (uint64_t)sampleRate * secondsToRecord;

    // --- Encoder WAV ---
    if (!initWavEncoder())
        return false;        

    // --- Configuración dispositivo ---
    if(!initRecorder()) {
        
        ma_device_uninit(&device_);
        ma_encoder_uninit(&rec_ctx_.encoder);

        return false;
    }

    rec_ctx_.recording = true;
    return true;
}

bool AudioInputModule::StopRec() {
    if (!rec_ctx_.recording.exchange(false))
        return true;

    ma_device_stop(&device_);
    ma_device_uninit(&device_);
    ma_encoder_uninit(&rec_ctx_.encoder);
    std::cout << "[AudioInputModule]    Stopping record..." << std::endl;
    std::cout << "[AudioInputModule]    Recorded " << rec_ctx_.framesWritten.load() / sampleRate;
    std::cout << " seconds to " << rec_ctx_.filename << std::endl;
    return true;
}

bool AudioInputModule::isRecording() {
    return rec_ctx_.recording.load();
}


bool AudioInputModule::initWavEncoder() {
    ma_encoder_config encoderConfig =
        ma_encoder_config_init( ma_encoding_format_wav,
                                ma_format_s16,
                                channels,
                                sampleRate);

    ma_result res = ma_encoder_init_file(rec_ctx_.filename.c_str(), &encoderConfig, &rec_ctx_.encoder);
    if (res != MA_SUCCESS) {
        std::cout << "[AudioInputModule]    Failed to initialize encoder. Error code: " << res << std::endl;
        return false;
    }

    return true;
}

bool AudioInputModule::initRecorder() {
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

    deviceConfig.capture.format     = ma_format_s16;
    deviceConfig.capture.channels   = channels;
    deviceConfig.sampleRate         = sampleRate;
    deviceConfig.dataCallback       = dataCallback;
    deviceConfig.pUserData          = &rec_ctx_;
    
    std::cout << "[AudioInputModule]    Initializating capture device..." << std::endl;
    if (ma_device_init(snd_ctx_, &deviceConfig, &device_) != MA_SUCCESS) {
        std::cerr << "[AudioInputModule] Failed to initialize capture device." << std::endl;
        ma_encoder_uninit(&rec_ctx_.encoder);
        return false;
    }

    std::cout << "[AudioInputModule]    Starting capturing device..." << std::endl;
    if (ma_device_start(&device_) != MA_SUCCESS) {
        std::cerr << "[AudioInputModule] Failed to start capture device." << std::endl;
        ma_device_uninit(&device_);
        ma_encoder_uninit(&rec_ctx_.encoder);
        return false;
    }

    return true;
}

void AudioInputModule::dataCallback(
    ma_device*  device, 
    void*       output, 
    const void* input, 
    ma_uint32   frameCount) 
{
    RecordingContext* ctx = (RecordingContext*)device->pUserData;
    if (!input || !ctx->recording) return;

    uint64_t framesWritten      = ctx->framesWritten.load();
    uint64_t framesRemaining    = ctx->maxFrames - framesWritten;
    uint64_t framesToWrite      = (frameCount > framesRemaining) ? framesRemaining : frameCount;

    if (framesToWrite > 0) {
        ma_uint64 actuallyWritten = 0;
        if (ma_encoder_write_pcm_frames(&ctx->encoder, input, framesToWrite, &actuallyWritten) == MA_SUCCESS)
            ctx->framesWritten += actuallyWritten;
    }

    // Límite automático: Marca y parada del dispositivo.
    if (ctx->framesWritten >= ctx->maxFrames) {
        ctx->recording = false;
    }
    (void)output;
}
