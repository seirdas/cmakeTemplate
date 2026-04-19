
#include "sound/AudioInputModule.hpp"
#include <system/sys.hpp>

// General ------------------------------------------------------------------------------

AudioInputModule::AudioInputModule(ma_context* ctx)
    : snd_ctx_(ctx)
{
    rec_ctx_.recording = false;
};

AudioInputModule::~AudioInputModule() {

    // Parar grabaciones activas
    if(isRecording()){
        SYS_INFO("AudioInputModule", "Stopping running recorders...");
        StopRec();
    } 
};


// Grabación ----------------------------------------------------------------------------

bool AudioInputModule::StartRec(std::string const& filename) {

    if (rec_ctx_.recording)
        return false;

    SYS_INFO("AudioInputModule", "Start recording to " + filename + ".wav ...");

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
    SYS_INFO("AudioInputModule", "Stopping record...");
    SYS_INFO("AudioInputModule", "Recorded " + std::to_string(rec_ctx_.framesWritten.load() / sampleRate));
    SYS_INFO("AudioInputModule", " seconds to " + rec_ctx_.filename);
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
        SYS_INFO("AudioInputModule", "Failed to initialize encoder. Error code: " + std::to_string(res));
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
    
    SYS_INFO("AudioInputModule", "Initializating capture device...");
    if (ma_device_init(snd_ctx_, &deviceConfig, &device_) != MA_SUCCESS) {
        SYS_WARN("AudioInputModule","Failed to initialize capture device.");
        ma_encoder_uninit(&rec_ctx_.encoder);
        return false;
    }

    SYS_INFO("AudioInputModule", "Starting capturing device...");
    if (ma_device_start(&device_) != MA_SUCCESS) {
        SYS_WARN("AudioInputModule","Failed to start capture device.");
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
