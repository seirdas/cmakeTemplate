#pragma once

#include "sound/AudioPlaybackModule.hpp"
#include <vector>

class PlaybackMorse : public AudioPlaybackModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor del módulo.
     *  Utiliza el constructor de la clase padre
     * @param ctx (ma_context*) Contexto de mini audio.
     * @param device_info (ma_device_info*) Información del dispositivo de audio.
     */
    PlaybackMorse(void* ctx, const void* device_info);

    /**
     * @brief Destructor del módulo
     *  Utiliza el destructor de la clase padre
     */
    ~PlaybackMorse() override = default;


// Ejecución ----------------------------------------------------------------------------

     /**
     * @brief Genera y reproduce un mensaje en morse a partir de un texto.
     * @details El propio módulo convierte el texto a pitidos y lo gestiona igual que un
     *  audio de archivo (identificado por @p audioName).
     * @param texto Texto a codificar (letras/números soportados por el diccionario Morse).
     * @param audioName Nombre con el que se identifica este sonido.
     * @param volume Volumen del sonido (0 a 100).
     * @param loop Modo de repetición.
     * @return true si se ha generado y empezado a reproducir correctamente, false en caso contrario.
     */
    bool playMorse(
        std::string const& texto,
        std::string const& audioName,
        unsigned short     volume       = 100,
        bool               loop         = false
    );


// Parámetros del módulo ----------------------------------------------------------------

    void setToneFrequency(float hz);
    void setPuntoMs(unsigned int time_ms);
    void setRayaMs(unsigned int time_ms);
    void setEspacioEntreSimbolos(unsigned int time_ms);
    void setEspacioEntreLetras(unsigned int time_ms);
    void setEspacioEntreMorse(unsigned int time_ms);
    void setSampleRate(unsigned int hz);



private:

// Generación ---------------------------------------------------------------------------

   /**
     * @brief Genera el audio (PCM mono, float 32) correspondiente a un texto en morse.
     * @param texto Texto a codificar (letras/números soportados por el diccionario Morse)
     * @return Vector de muestras PCM (mono). Vacío si no se generó nada.
     */
    std::vector<float> generate_morse_audio(std::string const& texto);

/************ Variables ****************************************************************/

// Parámetros de morse
    float              frequency_Hz_;           ///< Frecuencia del tono (Hz)
    unsigned int       punto_Ms_;               ///< Duración del punto (ms).
    unsigned int       raya_Ms_;                ///< Duración de la raya (ms).
    unsigned int       espacioEntreSimbolos_;   ///< Silencio entre símbolos de la misma letra (ms).
    unsigned int       espacioEntreLetras_;     ///< Silencio entre letras de la misma palabra (ms).
    unsigned int       espacioEntreMorse_;      ///< Duración del silencio entre palabras (loop) (ms).
    unsigned int       sampleRate_;             ///< Frecuencia de muestreo (Hz) del audio generado.

};
