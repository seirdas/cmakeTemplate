#pragma once

#include <thread>               // num_threads_
#include <unordered_map>
#include <vector>
#include <condition_variable>
#include <filesystem>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


// Declaración como estructura para evitar includes en hpp
struct SherpaOnnxOfflineTts;

/**
 * @brief Clase TTS para generar audios usando la librería Sherpa TTS
 * @note Voices downloaded from https://k2-fsa.github.io/sherpa/onnx/tts/all/index.html
 * @see lib-sherpaonnx.cmake
 */
class TTSMgr {

public:
    
// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de TTSMgr. Recibe el número de hilos a usar para la generación de audios.
     * @param num_threads_ Número de hilos a usar para la generación de audios.
     */
    TTSMgr(std::size_t const& num_threads_ = std::thread::hardware_concurrency());

    /**
     * @brief Destructor de TTSMgr.
     */
    ~TTSMgr();

    
// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa los módulos TTS cargando los modelos de voz.
     * @details Carga todos los modelos compatibles de ./VOICES_PATH
     * @note La macro VOICES_PATH se obtiene del toolchain para evitar duplicados
     * @return true si la inicialización fue exitosa, false en caso de error.
     */
    bool init(json* config);

    /**
     * @brief Libera los recursos asociados a los modelos de voz cargados.
     */
    void cerrar();

    /**
     * @brief recarga los modelos de voz (cierra, borra y vuelve a inicializar)
     */
    void reload();

    /**
     * @brief Intenta cargar los modelos disponibles que no estén cargados
     */
    void loadMissingModels();

    /**
     * @brief Devuelve si el módulo tiene algún proceso activo.
     */
    bool isWorking() const;


// Datos de modelos disponibles/cargados ------------------------------------------------

    /**
     * @brief Obtiene una lista con los nombres de los modelos disponibles
     */
    std::vector<std::string> getAvailableModels();
      
    /**
     * @brief Obtiene una lista con las rutas de los modelos disponibles
     */
    std::vector<std::string> getAvailableModelsPath();

    /**
     * @brief Obtiene una lista con los nombres de los modelos cargados
     * @note Con lazy_load activo, los modelos disponibles se consideran como cargados
     */
    std::vector<std::string> getLoadedModels() const;

    /**
     * @brief Obtiene la ruta de un modelo
     * @param model Nombre del modelo a obtener la ruta
     * @return Ruta del modelo
     */
    std::string getModelPath(std::string model) const;
      
    /**
     * @brief Obtiene el número de modelos disponibles
     */
    short numAvailableModels() const;

    /**
     * @brief Obtiene el número de modelos cargados
     * @note Con lazy_load activo, NO CUENTA los modelos no disponibles
     */
    short numLoadedModels() const;


// Control de modelos individuales ------------------------------------------------------

    /**
     * @brief Genera un audio a partir de un texto usando el modelo de voz especificado.
     * @param modelName Nombre del modelo (nombre de carpeta sin vits-piper-*)
     * @param text El texto a convertir en audio.
     * @param wavname El nombre del archivo WAV de salida (incluyendo la extensión wav).
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool generate(std::string const& modelName, std::string const& text, std::string const& wavname);

    /**
     * @brief Obtiene la frecuencia de muestreo del modelo de voz.
     * @param modelName Nombre del modelo (nombre de la carpeta).
     * @return Frecuencia de muestreo en Hz, o 0 si el modelo no está cargado.
     */
    int getSampleRate(std::string const& modelName) const;

    /**
     * @brief Obtiene el número de speakers (hablantes) del modelo de voz.
     * @param modelName Nombre del modelo (nombre de la carpeta).
     * @return Número de hablantes disponibles, o 0 si el modelo no está cargado.
     */
    int getNumSpeakers(std::string const& modelName) const;

    /**
     * @brief Devuelve el texto que está siendo procesado actualmente por el modelo.
     * @param modelName Nombre del modelo (nombre de la carpeta).
     * @return Cadena de texto en proceso de síntesis, o un string vacío si no está activo.
     */
    std::string getProccesingText(std::string const& modelName) const;

    /**
     * @brief Obtiene el nombre identificativo del modelo a partir de su ruta absoluta.
     * @details Busca el archivo con extensión `.onnx` dentro del directorio para extraer su stem.
     * @param modelAbsPath Ruta absoluta de la carpeta contenedora del modelo VITS.
     * @return Nombre del modelo, o un string vacío si no se encuentra ningún archivo `.onnx`.
     */
    std::string getModelName(std::filesystem::path modelAbsPath) const;


private:

// Carga de configuración ---------------------------------------------------------------

    // #TODO doxy
    void loadConfig(json* config);

// Inicialización de modelos ------------------------------------------------------------

    /**
     * @brief Carga de manera masiva todos los modelos disponibles en la ruta de voces.
     * @details Dependiendo de la configuración, realiza la carga de forma consecutiva 
     * o concurrente distribuyendo la tarea en hilos independientes.
     */
    void loadModels();

    /**
     * @brief Carga e inicializa en memoria un modelo VITS TTS específico a través de la API de Sherpa-Onnx.
     * @note Este método requiere que el hilo verifique primero si el sistema no está en proceso de parada.
     * @param modelAbsPath Ruta absoluta de la carpeta que contiene los ficheros del modelo (`.onnx`, `tokens.txt`, etc.).
     * @return @c true si el modelo se inicializó y registró correctamente en el mapa; @c false en caso contrario.
     */
    bool load_vits_model(std::filesystem::path modelAbsPath);

    /**
     * @brief Comprueba la integridad estructural de la carpeta de un modelo antes de intentar cargarlo.
     * @details Verifica que existan de manera simultánea el archivo `.onnx`, el fichero `tokens.txt` 
     * y el directorio de fonemas `espeak-ng-data`.
     * @param modelAbsPath Ruta absoluta de la carpeta que se va a evaluar.
     * @return `true` si contiene todos los elementos mínimos requeridos; `false` si falta alguno.
     */
    bool checkAvailableModel(std::filesystem::path modelAbsPath) const;

    /**
     * @brief Descarga un modelo específico liberando sus recursos asociados en memoria.
     * @details Si `lazy_load_` está activo, el modelo se desvincula de Sherpa pero mantiene su slot 
     * en el mapa como puntero nulo para permitir futuras recargas bajo demanda. Si está desactivado, 
     * el modelo se elimina por completo del mapa.
     * @param modelName Nombre del modelo que se desea descargar.
     * @return `true` si el modelo existía y fue descargado con éxito (o ya era nulo); `false` si no se encontró.
     */
    bool unload_model(std::string const& modelName);

    /**
     * @brief Hilo de ejecución (Worker) encargado de la gestión Keep-Alive de los modelos.
     * @details Monitoriza periódicamente el mapa de tiempos de último uso (`last_used_`). Si un modelo 
     * supera el tiempo de inactividad programado en `keep_alive_time_` y no se encuentra retenido 
     * por ninguna tarea activa de generación, invoca a `unload_model` de forma segura liberando la memoria.
     */
    void keepAliveWorker();


/************ Variables ********************************************************/

    using TTSModelsMap  = std::unordered_map<std::string, const SherpaOnnxOfflineTts*>;
    using TTSTextsMap   = std::unordered_map<std::string, std::string>; // Podría ser un struct con más datos
    using TTSTimeMap    = std::unordered_map<std::string, std::chrono::steady_clock::time_point>;

    // Modelos de voz
    TTSModelsMap            loaded_models_;         ///< Mapa de modelos TTS cargados
    short                   num_available_models_;  ///< Número de modelos disponibles
    size_t                  num_threads_;           ///< Número de hilos con los que se generarán los audios
    bool                    concurrent_init_;       ///< Activa/desactiva la inicialización concurrente (experimental)
    std::string             models_path_;           ///< Ruta de carpetas donde residen los modelos
    mutable std::mutex      models_mutex_;          ///< Mutex para proteger el mapa de modelos
    
    // Lazy Load (Carga modelo solo al usarlo, se descarga en un tiempo)
    bool                    lazy_load_;             ///< Activa/desactiva la inicialización solo al usar un modelo
    std::chrono::seconds    keep_alive_time_;       ///< Tiempo de vida en segundos de los modelos cargados con lazyload
    TTSTimeMap              last_used_;             ///< Mapa de relación entre modelo - ultimo tiempo usado
    std::thread             keepalive_thread_;      ///< Hilo reaper para matar los modelos que superan el tiempo de vida
    std::mutex              keepalive_mtx_;         ///< Mutex para el acceso al mapa last_used
    std::condition_variable keepalive_cv_;          ///< Condition variable para el acceso al hilo reaper keepalive_thread

    // Datos de modelos
    TTSTextsMap             processing_texts_;      ///< Relaciona modelo - texto que está procesando
    mutable std::mutex      processing_mtx_;        ///< Mutex para proteger el acceso al mapa (mutable para métodos const)

    // Datos del modulo TTS
    std::atomic<short>      active_tasks_;      ///< Indica si hay algo en ejecución
    unsigned short          num_load_retries_;  ///< Número máximo de reintentos para recargar modelos que fallaron al cargar
    std::atomic<bool>       running_;           ///< Indica si no se ha comandado destruir la clase
    std::atomic<bool>       loading_;           ///< Indica si está cargando modelos
    std::mutex              exit_mtx_;          ///< Evita destruir TTSMgr si hay algo ejecutándose
    std::condition_variable exit_cv_;           ///< Notifica cuándo paran las tareas
};
