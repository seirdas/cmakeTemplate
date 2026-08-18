//creo la clase de los inputs
#pragma once

#include <string>               // std::string
#include <vector>               // Vectores
#include <memory>               // unique_ptr
#include <atomic>
#include <functional>           // Callback expuesto de frames de audio (hacia afuera)
#include <mutex>


/**
 * @class AudioCaptureModule
 * @brief Clase para la captura de audio utilizando miniaudio.
 */
class AudioCaptureModule {

public: 

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de AudioCaptureModule.
     * @param ctx Contexto de mini audio.
     * @param device_info Información del dispositivo de audio.
     */
    AudioCaptureModule(std::string const& moduleName, void* ctx, const void* device_info);

    /**
     * @brief Destructor de AudioCaptureModule.
     */
    ~AudioCaptureModule();

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    AudioCaptureModule(AudioCaptureModule const&) = delete;
    AudioCaptureModule& operator=(AudioCaptureModule const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    AudioCaptureModule(AudioCaptureModule&&) = delete;
    AudioCaptureModule& operator=(AudioCaptureModule&&) = delete;


// Inicialización -------------------------------------------------------------------

    /**
     * @brief Inicializa el dispositivo de captura
     * @note Aparte de la inicialización, también lo pone a capturar audio (start)
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config = nullptr, std::string const& captureName = "");

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);

    /**
     * @brief Detiene la captura de audio y desinicializa el dispositivo
     *  Si estaba grabando, deja de grabar y guarda lo grabado.
     */
    void close();

    /**
     * @brief Desinicializa y cierra el módulo 
     *  y lo vuelve a inicializar con los nuevos parámetros
     */
    bool reload();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa la captura de audio
     * @note Devolverá true si el módulo ya estaba capturando (no hace nada)
     * @return @c true Si se ha iniciado la captura correctamente, @c false en caso contrario
     */
    bool startCapture();

    /**
     * @brief Detiene la captura de audio
     * @note Devolverá true si el módulo no estaba capturando (no hace nada)
     * @return @c true Si se ha detenido la captura correctamente, @c false en caso contrario
     */
    bool stopCapture();
    
    
// Parámetros del módulo ----------------------------------------------------------------
    
    /**
     * @brief Devuelve el nombre del dispositivo
     * @return Nombre del dispositivo
     */
    std::string getDeviceName() const;

    /**
     * @brief Obtiene el nombre de este AudioCaptureModule
     * @return Nombre de este componente 
     */
    std::string getModuleName() const;

    /**
     * @brief Obtiene el número de canales totales del dispositivo de captura 
     * @return Número de canales totales del dispositivo de captura
     */
    unsigned short getNumChannels() const;

    /**
     * @brief Obtiene el canal seleccionado
     * @return Canal seleccionado
     */
    unsigned short getSelectedChannel() const;

    /**
     * @brief Obtiene la frecuencia de muestreo de captura 
     * @return Frecuencia de muestreo
     */
    unsigned int getSampleRate() const;

    /**
     * @brief Establece un (nuevo) dispositivo de captura
     *  Si la captura estaba inicializada, cierra y vuelve a inicializar
     * @details Busca la información del dispositivo de captura (ma_device_info)
     *  a partir del nombre, fallando si no existe (hace un updateDevices simplificado)
     * @param deviceName Nuevo nombre de dispositivo
     * @return @c true Si se ha podido reiniciar correctamente o no ha habido reinicialización
     *  @c false si ha fallado la reinicialización 
     */
    bool setDeviceName(std::string const& deviceName);

    /**
     * @brief Establece un (nuevo) número de canales
     *  Si la captura estaba inicializada, cierra y vuelve a inicializar
     * @param numChannels Nuevo número de canales
     * @return @c true Si se ha podido reiniciar correctamente o no ha habido reinicialización
     *  @c false si ha fallado la reinicialización 
     */
    bool setNumChannels(unsigned short numChannels);

    /**
     * @brief Establece una (nueva) frecuencia de muestreo
     *  Si la captura estaba inicializada, cierra y vuelve a inicializar
     * @param numChannels Nueva frecuencia de muestreo
     * @return @c true Si se ha podido reiniciar correctamente o no ha habido reinicialización
     *  @c false si ha fallado la reinicialización 
     */
    bool setSampleRate(unsigned int sampleRate);

    /**
     * @brief Establece el canal seleccionado 
     * @param selectedChannel Canal seleccionado
     * @param @c false si el canal seleccionado > canales disponibles, @c true si se puede seleccionar
     */
    bool setSelectedChannel(unsigned short selectedChannel);

    /**
     * @brief Indica si el dispositivo está activo o se ha desconectado
     * @return @c true Si el dispositivo está activo, @c false en caso contrario 
     */
    bool isValid() const;


// Captura ------------------------------------------------------------------------------

    /**
     * @brief Devuelve el valor medio RMS del buffer de captura.
     * @return Nivel de señal RMS del buffer de captura
     */
    float getRmsLevel() const;

    /**
     * @brief Devuelve el nivel del pico del buffer de captura.
     */
    float getPeakLevel() const;

    /**
     * @brief Obtener tamaño del buffer de captura
     * @return Tamaño de buffer de captura
     */
    size_t getBufferSize() const;


// Callback expuesto --------------------------------------------------------------------
    
    /**
     * @brief Alias para definir la firma del callback de audio.
     * Recibe un puntero a los datos PCM (int16_t) y la cantidad de muestras.
     */
    using AudioCallback = std::function<void(const int16_t* data, size_t size)>;

    /**
     * @brief Registra un callback externo para procesar las tramas de audio capturadas.
     * * Este método permite inyectar una función o lambda desde el exterior
     * para que sea ejecutada cada vez que el hilo de audio tenga nuevos datos disponibles.
     * * @param cb La función (o functor) que se ejecutará al recibir nuevos frames.
     */
    void setCallback_OnFrame(AudioCallback cb);

    /**
     * @brief Elimina la función asociada a onFrame_
     */
    void clearCallback_OnFrame();

    /**
     * @brief Indica si este modulo tiene una función inyectada para
     *  procesar muestras de audio
     */
    bool hasCallback_OnFrame();


// Grabación ----------------------------------------------------------------------------

    /**
     * @brief Comienza a grabar en el archivo especificado
     * @param filename Archivo donde se grabarán las muestras. 
     *  No es necesario especificar el formato, se graba en .wav
     * @return @c true si consigue grabar, @c false si falla
     */
    bool StartRec(std::string const& filename);

    /**
     * @brief Orden de para de grabar
     * @return @c true si se ha generado bien el archivo de la grabación, @c false en caso contrario
     */
    bool StopRec();

    /**
     * @brief Obtener tamaño del buffer de grabación
     * @return Tamaño de buffer de grabación
     */
    size_t getRecBufferSize() const;

    /** 
     * @brief Devuelve si el dispositivo de captura está grabando o no
     * @return @c true si está grabando, @c false en caso contrario
     */
    bool isRecording() const;


// Parámetros de suavizado de valores ---------------------------------------------------

    /**
     * @brief Activa/Desactiva los valores suavizados
     * @param value @c true para activar los valores suavizados, @c false para desactivar
     */
    void enableSmoothedValues(bool value);

    /**
     * @brief Set the SmoothAttackCoeff object
     * @param value Valor de ataque (+grande = subida lenta)
     */
    void setSmoothAttackCoeff(float value);

    /**
     * @brief Set the SmoothReleaseCoeff object
     * @param value Valor de release (+grande = bajada lenta)
     */
    void setSmoothReleaseCoeff(float value);


private:

// Codificador de grabación -------------------------------------------------------------

    /**
     * @brief Inicializa el codificador para pasar a .wav
     * @note Hay que inicializar y desinicializar el encoder cada vez que quieras grabar en un archivo nuevo
     * @param filename Recibe el nombre del archivo donde se va a guardar el audio
     */
    bool init_rec_encoder(std::string const& filename);

    /**
     * @brief Desinicializa el encoder de grabación
     * @note Hay que inicializar y desinicializar el encoder 
     *  cada vez que se vaya a grabar en un archivo nuevo
    */
    void uninit_rec_encoder();

    /**
     * @brief Guarda el audio grabado en un fichero .wav
     * @details Por defecto se hace justo después de parar la grabación
     * @return @c true Si se ha generado bien el archivo, @c en caso de error
     */
    bool save_recording();


// Suavizado de niveles -----------------------------------------------------------------

    /**
     * @brief Calcula un valor suavizado respecto a su valor anterior
     * @param rawValue Valor "crudo" obtenido
     * @param previousValue Valor anterior
     * @param attackCoeff Coeficiente de "attack"
     * @param releaseCoeff Coeficiente de "release"
     * @return Valor suavizado
     */
    float smooth_level(
        float const     rawValue, 
        float const&    previousValue, 
        float           attackCoeff = 0, 
        float           releaseCoeff = 0
    );


/************ Variables ********************************************************/

// Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;               ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    std::string             name_;                ///< Nombre asociado al dispositivo de captura
    bool                    is_valid_;            ///< Bandera para indicar si está inicializado el dispositivo
    bool                    initialized_;         ///< Bandera para indicar inicialización exitosa
    bool                    running_;             ///< Flag de captura corriendo

// Configuración de entrada
    unsigned short          channels_;            ///< Canales del audio
    unsigned int            sampleRate_;          ///< Frecuencia de muestreo
    std::string             deviceName_;          ///< Nombre del dispositivo de captura
    unsigned short          selectedChannel_;     ///< Número de canal seleccionado. Si es 0, usa todos los canales

// Grabación 
    std::vector<int16_t>    rec_buffer_;          ///< Buffer que acumula las muestras de audio capturadas para grabación (formato s16)
    bool                    codec_inited_;        ///< Flag que indica si el encoder se ha inicializado
    std::atomic<bool>       recording_;           ///< Flag para guardar samples de audio en el buffer de grabación
    std::string             rec_filename_;        ///< Nombre de archivo generado con la grabación (para INFO)
    
// Captura
    const int16_t           max_int16_val_;       ///< Máximo valor del tipo int16_t
    std::atomic<float>      rmsLevel_;            ///< Nivel actual de señal (RMS o pico según usePeak_)
    std::atomic<float>      peakLevel_;           ///< Nivel de pico (solo cuando usePeak_ == true)
    std::vector<int16_t>    captureBuffer_;       ///< Buffer que acumula las muestras de audio capturadas (formato s16)
    unsigned int            processBufferSize_;   ///< frames por bloque de proceso
    
    // Suavizado de valores
    bool                    smoothedValues_;      ///< Suaviza los valores de captura (RMS, Peak...)
    float                   attackCoeff_;         ///< Valor de ataque (+grande = subida lenta)
    float                   releaseCoeff_;        ///< Valor de release (+grande = bajada lenta)

// Función inyectada
    AudioCallback           onFrame_cb_;          ///< Almacena la función de callback registrada externamente
    mutable std::mutex      onFrame_mtx_;         ///< Mutex para acceso a la función onFrame callback

};
