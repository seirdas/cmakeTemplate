#include "sound/SoundMgr.hpp"
#include <iostream>

// General ------------------------------------------------------------------------------

SoundMgr::SoundMgr() : engine_initialized_(false) {

}

SoundMgr::~SoundMgr() {
    stop();
}

bool SoundMgr::init() {
    std::cout << "[SoundMgr]    Initializating sound engine..." << std::endl;

    ma_result res = ma_engine_init(NULL, &engine_);
    engine_initialized_ = (res == MA_SUCCESS) ? true : false;

    return engine_initialized_;
}

bool SoundMgr::stop() {

    // No hacer nada si ya se ha cerrado.
    if (!engine_initialized_) return true;

    std::cout << "[SoundMgr]    Closing sound engine..." << std::endl;
    
    // Parar grabaciones activas
    if(isRecording()){
        std::cout << "[SoundMgr]    Stopping running recorders..." << std::endl;
        StopRec();
    } 
    
    // Cerrar el motor de audio
    if (engine_initialized_){
        ma_engine_uninit(&engine_);
        engine_initialized_ = false;
    }

    return true;
}

void SoundMgr::test() {
    ma_engine_play_sound(&engine_, "DefaultDance.mp3", NULL);
}

void SoundMgr::test2() {
    ma_engine_play_sound(&engine_, "chinos.mp3", NULL);
}


// Grabación ----------------------------------------------------------------------------

bool SoundMgr::StartRec(std::string const& filename) {

    if (ctx_.recording)
        return false;

    std::cout << "[SoundMgr]    Start recording to " << filename << ".wav ..." << std::endl;

    ctx_.filename = filename+".wav";
    ctx_.framesWritten.store(0);
    ctx_.maxFrames = (uint64_t)sampleRate * secondsToRecord;

    // --- Encoder WAV ---
    if (!initWavEncoder())
        return false;        

    // --- Configuración dispositivo ---
    if(!initRecorder()) {
        
        ma_device_uninit(&device_);
        ma_encoder_uninit(&ctx_.encoder);

        return false;
    }

    ctx_.recording = true;
    return true;
}

bool SoundMgr::StopRec() {
    if (!ctx_.recording.exchange(false))
        return true;

    ma_device_stop(&device_);
    ma_device_uninit(&device_);
    ma_encoder_uninit(&ctx_.encoder);
    std::cout << "[SoundMgr]    Stopping record..." << std::endl;
    std::cout << "[SoundMgr]    Recorded " << ctx_.framesWritten.load() / sampleRate;
    std::cout << " seconds to " << ctx_.filename << std::endl;
    return true;
}

bool SoundMgr::isRecording() {
    return ctx_.recording.load();
}


bool SoundMgr::initWavEncoder() {
    ma_encoder_config encoderConfig =
        ma_encoder_config_init( ma_encoding_format_wav,
                                ma_format_s16,
                                channels,
                                sampleRate);

    ma_result res = ma_encoder_init_file(ctx_.filename.c_str(), &encoderConfig, &ctx_.encoder);
    if (res != MA_SUCCESS) {
        std::cout << "[SoundMgr]    Failed to initialize encoder. Error code: " << res << std::endl;
        return false;
    }

    return true;
}

bool SoundMgr::initRecorder() {
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);

    deviceConfig.capture.format     = ma_format_s16;
    deviceConfig.capture.channels   = channels;
    deviceConfig.sampleRate         = sampleRate;
    deviceConfig.dataCallback       = dataCallback;
    deviceConfig.pUserData          = &ctx_;
    
    std::cout << "[SoundMgr]    Initializating capture device..." << std::endl;
    if (ma_device_init(nullptr, &deviceConfig, &device_) != MA_SUCCESS) {
        std::cerr << "[SoundMgr] Failed to initialize capture device." << std::endl;
        ma_encoder_uninit(&ctx_.encoder);
        return false;
    }

    std::cout << "[SoundMgr]    Starting capturing device..." << std::endl;
    if (ma_device_start(&device_) != MA_SUCCESS) {
        std::cerr << "[SoundMgr] Failed to start capture device." << std::endl;
        ma_device_uninit(&device_);
        ma_encoder_uninit(&ctx_.encoder);
        return false;
    }

    return true;
}

void SoundMgr::dataCallback(
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
        ma_device_stop(device); 
    }
    (void)output;
}
