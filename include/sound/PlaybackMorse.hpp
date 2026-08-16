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

    /**
     * @brief Establece la frecuencia del tono de audio para la reproducción del código Morse.
     * @param hz Frecuencia en hercios (Hz). Valores comunes suelen estar entre 400 y 1000 Hz.
     */
    void setToneFrequency(float hz);

    /**
     * @brief Establece la duración en milisegundos correspondiente a un punto en el código Morse.
     * @param time_ms Duración del punto en milisegundos.
     */
    void setPuntoMs(unsigned int time_ms);

    /**
     * @brief Establece la duración en milisegundos correspondiente a una raya en el código Morse.
     * @param time_ms Duración de la raya en milisegundos (usualmente el triple de un punto).
     */
    void setRayaMs(unsigned int time_ms);

    /**
     * @brief Establece el tiempo de silencio entre símbolos (puntos y rayas) dentro de una misma letra.
     * @param time_ms Duración de la pausa en milisegundos.
     */
    void setEspacioEntreSimbolos(unsigned int time_ms);

    /**
     * @brief Establece el tiempo de silencio entre diferentes letras en una secuencia Morse.
     * @param time_ms Duración de la pausa en milisegundos.
     */
    void setEspacioEntreLetras(unsigned int time_ms);

    /**
     * @brief Establece el tiempo de silencio entre palabras o secuencias completas de código Morse.
     * @param time_ms Duración de la pausa en milisegundos.
     */
    void setEspacioEntreMorse(unsigned int time_ms);

    /**
     * @brief Establece la tasa de muestreo (sample rate) para la generación de audio de los tonos Morse.
     * @param hz Tasa de muestreo en hercios (Hz). Por ejemplo: 44100 o 48000 Hz.
     */
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
    unsigned int       punto_ms_;               ///< Duración del punto (ms).
    unsigned int       raya_ms_;                ///< Duración de la raya (ms).
    unsigned int       espacioEntreSimbolos_;   ///< Silencio entre símbolos de la misma letra (ms).
    unsigned int       espacioEntreLetras_;     ///< Silencio entre letras de la misma palabra (ms).
    unsigned int       espacioEntreMorse_;      ///< Duración del silencio entre palabras (loop) (ms).
    unsigned int       sampleRate_;             ///< Frecuencia de muestreo (Hz) del audio generado.

};
