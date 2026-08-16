#pragma once

#include "sound/AudioPlaybackModule.hpp"

class PlaybackFile : public AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor del módulo.
     *  Utiliza el constructor de la clase padre
     * @param ctx (ma_context*) Contexto de mini audio.
     * @param device_info (ma_device_info*) Información del dispositivo de audio.
     */
    PlaybackFile(void* ctx, const void* device_info);

    /**
     * @brief Destructor del módulo
     *  Utiliza el destructor de la clase padre
     */
    ~PlaybackFile() override = default;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Reproduce un archivo de audio buscándolo por nombre dentro de la carpeta
     *  configurada para este playback (ver @p audioFolder del constructor).
     * @param filename Nombre del archivo (ej. "ding.wav"), sin ruta.
     * @param volume Volumen del sonido (0 a 100)
     * @param loop Modo de repetición
     * @param forceStop Fuerza la parada si se desactiva (de lo contrario, deja terminar el wav)
     * @param pitch Tono del sonido ( 1.0f = "normal" )
     */
    void playFromFolder(
        std::string const&  filename,
        unsigned short      volume,
        bool                loop,
        bool                forceStop,
        unsigned short      pitch
    );


// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Establece la ruta de la carpeta de audios de este módulo
     * @param audioFolder Ruta de la carpeta de audios
     */
    void setAudioFolder(std::string const& audioFolder);


private:

/************ Variables ****************************************************************/

// Gestión de archivos de audio
    std::string             audioFolder_;           ///< Carpeta de archivos de audio de este playback (usada por playFromFolder)

};
