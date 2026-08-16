#include "sound/PlaybackFile.hpp"
#include "sound/AudioPlaybackModule.hpp"

#if defined MINIAUDIO || defined MINIAUDIO_VERSION

    #include "system/SystemMgr.hpp"


    // General ------------------------------------------------------------------------------

    PlaybackFile::PlaybackFile(void* ctx, const void* device_info)
        : AudioPlaybackModule(ctx, device_info) 
    {

    }


    // Ejecución ----------------------------------------------------------------------------

        void PlaybackFile::playFromFolder(
            std::string const&  filename,
            unsigned short      volume,
            bool                loop,
            bool                forceStop,
            unsigned short      pitch)
        {
            if (audioFolder_.empty()) {
                SYS_WARN("AudioPlaybackModule", "playFromFolder: audioFolder not defined");
                return;
            }

            // Construye la ruta completa a partir de la carpeta configurada y el nombre del archivo
            std::filesystem::path fullPath = std::filesystem::path(audioFolder_) / filename;

            AudioPlaybackModule::playAudio(fullPath.string(), volume, loop, forceStop, pitch);
        }

    // Parámetros del módulo ----------------------------------------------------------------

    void PlaybackFile::setAudioFolder(std::string const& audioFolder) {
        audioFolder_ = audioFolder;
    }

#else
// ============================================================
//  (Stubs)
// ============================================================

// General ------------------------------------------------------------------------------
PlaybackFile::PlaybackFile(void*, const void*) :
    AudioPlaybackModule(nullptr, nullptr)
{}

// Ejecución ----------------------------------------------------------------------------
void PlaybackFile::playFromFolder(
    std::string const&,
    unsigned short,
    bool,
    bool,
    unsigned short 
) { return; }

// Parámetros del módulo ----------------------------------------------------------------
void PlaybackFile::setAudioFolder(std::string const&)     { return; }

#endif