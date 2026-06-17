//creo la clase de los inputs
#pragma once

#include <string>               // std::string
#include <vector>               // Vectores
#include <memory>               // unique_ptr
#include <atomic>

/**
 * @class AudioInputModule
 * @brief Clase para la captura de audio utilizando miniaudio.
 */
class AudioInputModule{

public: 

    // General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioInputModule.
     * @param ctx Contexto de mini audio.
     * @param device_info Información del dispositivo de audio.
     */
    AudioInputModule(void* ctx, const void* devInfo);

    /**
     * @brief Destructor de AudioInputModule.
     */
    ~AudioInputModule();


    //Ejecución -----------------------------------------------------------------------------

    /**
     * @brief Inicializa el dispositivo de captura
     * @return true si arranca bien, false si no
     */
    bool init();

    /**
     * @brief Detiene la captura de audio y desinicializa el dispositivo
     *  Si estaba grabando, deja de grabar y guarda lo grabado.
     */
    void stop();

    
    // Información y parámetros -------------------------------------------------------------
    
    /**
     * @brief Devuelve el nombre del dispositivo
     * @return Nombre del dispositivo
     */
    std::string deviceName() const;

    /**
     * @brief Indica si el dispositivo está activo o se ha desconectado
     * @return @c true Si el dispositivo está activo, @c false en caso contrario 
     */
    bool isValid();


    // Captura ------------------------------------------------------------------------------

    /**
     * @brief Devuelve el valor medio RMS del buffer de captura.
     * @return Nivel de señal RMS del buffer de captura
     */
    float getRmsLevel() const;

    /**
     * @brief Obtener tamaño del buffer de captura
     * @return Tamaño de buffer de captura
     */
    size_t getBufferSize();


    // Grabación ----------------------------------------------------------------------------

    /**
     * @brief Comienza a grabar en el archivo especificado
     * @param filename Archivo donde se grabarán las muestras. 
     *  No es necesario especificar el formato, se graba en .wav
     * @return true si consigue grabar, false si falla
     */
    void StartRec(std::string const& filename);

    /**
     * @brief Orden de para de grabar
     * @return 
     */
    void StopRec();

    /**
     * @brief Obtener tamaño del buffer de grabación
     * @return Tamaño de buffer de grabación
     */
    size_t getRecBufferSize();

    /** 
     * @brief Devuelve si el dispositivo de captura está grabando o no
     * @return @c true si está grabando, @c false en caso contrario
     */
    bool isRecording();


private:

    // Codificador de grabación -------------------------------------------------------------

    /**
     * @brief Inicializa el codificador para pasar a .wav
     * @note Hay que inicializar y desinicializar el encoder cada vez que quieras grabar en un archivo nuevo
     * @param filename Recibe el nombre del archivo donde se va a guardar el audio
     */
    void InitRecEncoder(std::string const& filename);

    /**
     * @brief Desinicializa el encoder de grabación
     * @note Hay que inicializar y desinicializar el encoder cada vez que quieras grabar en un archivo nuevo
    */
    void UninitRecEncoder();

    /**
     * @brief Guarda el audio grabado en un fichero .wav
     * @details Por defecto se hace justo después de parar la grabación
     */
    void saveRecording();


/************ Variables ********************************************************/

    // Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;               ///< Miembros dependientes de la librería externa

    // Configuración de entrada
    unsigned int            channels_;            ///< Canales del audio
    unsigned int            sampleRate_;          ///< Frecuencia de muestreo
    std::vector<int16_t>    buffer_;              ///< Buffer que acumula las muestras de audio capturadas (formato s16)
    unsigned int            processBufferSize_;   ///< frames por bloque de proceso

    // Grabación 
    std::vector<int16_t>    rec_buffer_;          ///< Buffer que acumula las muestras de audio capturadas para grabación (formato s16)
    bool                    codec_inited_;        ///< Flag que indica si el encoder se ha inicializado
    std::atomic<bool>       recording_;           ///< Flag para guardar samples de audio en el buffer de grabación
    std::string             rec_filename_;        ///< Nombre de archivo generado con la grabación (para INFO)
    
    // Datos del dispositivo  
    bool                    is_valid_;            ///< Bandera para indicar si está inicializado el dispositivo
    std::string             deviceName_;          ///< Nombre del dispositivo

    // Valor de entrada del RMS
    std::atomic<float>      rmsLevel_;            ///< guarda el nivel actual de rms
};
