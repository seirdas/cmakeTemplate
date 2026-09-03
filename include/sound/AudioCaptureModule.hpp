#pragma once

#include <string>               // std::string
#include <memory>               // unique_ptr
#include <functional>           // Callback expuesto de frames de audio (hacia afuera)
#include <mutex>
#include <string>


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
    AudioCaptureModule(std::string const& moduleName, void* ctx);

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
    bool init(void* config = nullptr);

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
    bool close();

    /**
     * @brief Desinicializa y cierra el módulo 
     *  y lo vuelve a inicializar con los nuevos parámetros
     */
    bool reload();


// Dispositivos del módulo --------------------------------------------------------------

    /**
     @brief Añade y configura un nuevo dispositivo de captura de audio al módulo.
     @details Resuelve la información del dispositivo mediante el callback de resolución, valida los canales disponibles,
              configura el enrutamiento de canales de miniaudio (soporta canales dedicados o mezcla general) e inicializa
              el motor de audio asociado.
     @param deviceName Nombre del dispositivo físico a buscar (ej: nombre devuelto por el backend de audio).
     @param channelSelected Índice del canal específico a utilizar (1 basado). Si se pasa 0, se habilitan todos los canales nativos.
     @param deviceAlias Nombre identificativo único (alias) opcional para el dispositivo.
     @return @c true si el dispositivo se añadió e inicializó correctamente; @c false en caso contrario.
     */
    bool addCaptureDevice(
        std::string const&  deviceName, 
        unsigned int        channelSelected,
        unsigned int        sampleRate = 48000,
        std::string const&  deviceAlias = ""
    );

    /**
     @brief Elimina un dispositivo de reproducción activo del módulo a partir de su alias.
     @details Detiene y desinicializa de forma síncrona todos los sonidos y buffers asociados al dispositivo especificado,
              libera los recursos de miniaudio (engine y device) y retira la instancia del registro interno de forma segura.
     @param deviceAlias Nombre identificativo (alias) del dispositivo de reproducción que se desea eliminar.
     @return true si el dispositivo fue encontrado y eliminado con éxito; false si el alias no existe o falló la operación.
     */
    bool removeCaptureDevice(std::string const& deviceAlias);

    /**
     * @brief Inyecta la función callback para resolver información de dispositivos de audio.
     * @param fn Función callback de tipo DeviceResolveFunction.
     */
    void setCallback_onDeviceResolve(std::function<const void*(std::string&)> cb);

    /**
     * @brief Elimina o resetea el callback de resolución de dispositivos.
     */
    void clearCallback_onDeviceResolve();

    /**
     * @brief Comprueba si el callback de resolución de dispositivos está definido.
     * @return true si hay un callback asignado; false en caso contrario.
     */
    bool hasCallback_onDeviceResolve() const;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa la captura de audio
     * @note Devolverá true si el módulo ya estaba capturando (no hace nada)
     * @return @c true Si se ha iniciado la captura correctamente, @c false en caso contrario
     */
    bool startCapture(std::string const& deviceAlias);

    /**
     * @brief Detiene la captura de audio
     * @note Devolverá true si el módulo no estaba capturando (no hace nada)
     * @return @c true Si se ha detenido la captura correctamente, @c false en caso contrario
     */
    bool stopCapture(std::string const& deviceAlias);
    
    
// Parámetros del módulo ----------------------------------------------------------------

    /**
     * @brief Obtiene el nombre de este AudioCaptureModule
     * @return Nombre de este componente 
     */
    std::string getModuleName() const;

    /**
     * @brief Obtiene la frecuencia de muestreo de captura 
     * @return Frecuencia de muestreo
     */
    unsigned int getSampleRate(std::string const& deviceAlias) const;

    /**
     * @brief Indica si el dispositivo está activo o se ha desconectado
     * @return @c true Si el dispositivo está activo, @c false en caso contrario 
     */
    bool isValid(std::string const& deviceAlias) const;


// Captura ------------------------------------------------------------------------------

    /**
     * @brief Devuelve el valor medio RMS del buffer de captura.
     * @return Nivel de señal RMS del buffer de captura
     */
    float getRmsLevel(std::string const& deviceAlias) const;

    /**
     * @brief Devuelve el nivel del pico del buffer de captura.
     */
    float getPeakLevel(std::string const& deviceAlias) const;

    /**
     * @brief Obtener tamaño del buffer de captura
     * @return Tamaño de buffer de captura
     */
    size_t getBufferSize(std::string const& deviceAlias) const;


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
    bool StartRec(std::string const& deviceAlias, std::string const& filename);

    /**
     * @brief Orden de para de grabar
     * @return @c true si se ha generado bien el archivo de la grabación, @c false en caso contrario
     */
    bool StopRec(std::string const& deviceAlias);

    /**
     * @brief Obtener tamaño del buffer de grabación
     * @return Tamaño de buffer de grabación
     */
    size_t getRecBufferSize(std::string const& deviceAlias) const;

    /** 
     * @brief Devuelve si el dispositivo de captura está grabando o no
     * @return @c true si está grabando, @c false en caso contrario
     */
    bool isRecording(std::string const& deviceAlias) const;


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
    bool init_rec_encoder(std::string const& filename, void* devicePtr);

    /**
     * @brief Desinicializa el encoder de grabación
     * @note Hay que inicializar y desinicializar el encoder 
     *  cada vez que se vaya a grabar en un archivo nuevo
    */
    void uninit_rec_encoder(void* devicePtr);

    /**
     * @brief Guarda el audio grabado en un fichero .wav
     * @details Por defecto se hace justo después de parar la grabación
     * @return @c true Si se ha generado bien el archivo, @c en caso de error
     */
    bool save_recording(void* devicePtr);


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

// Alias
    using DeviceResolveFunction = std::function<const void*(std::string&)>;

// Estructura PIMPL para no depender de la librería en el header
    struct Impl;
    std::unique_ptr<Impl>   pimpl_;               ///< Miembros dependientes de la librería externa

// Inicialización y ejecución
    std::string             name_;                ///< Nombre asociado al dispositivo de captura
    bool                    is_valid_;            ///< Bandera para indicar si está inicializado el dispositivo
    bool                    initialized_;         ///< Bandera para indicar inicialización exitosa
    bool                    running_;             ///< Flag de captura corriendo

// Resolver dispositivos
    DeviceResolveFunction   onDeviceResolve_ = nullptr; ///< Callback para la resolución de dispositivos
    mutable std::mutex      onDeviceResolve_mtx_;       ///< Mutex para proteger el acceso concurrente al callback

// Captura
    const int16_t           max_int16_val_;       ///< Máximo valor del tipo int16_t
    unsigned int            processBufferSize_;   ///< frames por bloque de proceso
    
// Suavizado de valores
    bool                    smoothedValues_;      ///< Suaviza los valores de captura (RMS, Peak...)
    float                   attackCoeff_;         ///< Valor de ataque (+grande = subida lenta)
    float                   releaseCoeff_;        ///< Valor de release (+grande = bajada lenta)

};
