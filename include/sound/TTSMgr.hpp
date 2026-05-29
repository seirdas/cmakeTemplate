#pragma once

#include <thread>               // num_threads_
#include <unordered_map>
#include <vector>
#include <condition_variable>
#include <filesystem>

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
    bool init();

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

// Datos de los modelos ---------------------------------------------------------

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
     */
    short numLoadedModels() const;


// Datos y control de modelos -----------------------------------------------------------

    /**
     * @brief Genera un audio a partir de un texto usando el modelo de voz especificado.
     * @param modelName Nombre del modelo que genera el audio
     * @param text El texto a convertir en audio.
     * @param wavname El nombre del archivo WAV de salida (incluyendo la extensión wav).
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool generate(std::string const& modelName, std::string const& text, std::string const& wavname);

    /**
     * @brief Obtiene la frecuencia de muestreo del modelo de voz
     */
    int getSampleRate(std::string const& modelName) const;

    /**
     * @brief Obtiene el número de speakers (hablantes) del modelo de voz
     */
    int getNumSpeakers(std::string const& modelName) const;

    /**
     * @brief Devuelve el texto que está siendo procesado por el modelo
     */
    std::string getProccesingText(std::string const& modelName) const;


private:

// Inicialización de modelos ------------------------------------------------------------

    /**
     * @brief Carga un modelo vits TTS 
     */
    bool load_vits_model(std::filesystem::path modelAbsPath);

    /**
     * @brief Libera/Borra un modelo cargado
     * @details Esto se usará en debug y demás, no creo que sea necesario en ejecución normal.
     */
    bool unload_model(std::string const& modelName);


/************ Variables ********************************************************/

    using TTSModelsMap  = std::unordered_map<std::string, const SherpaOnnxOfflineTts*>;
    using TTSTextsMap   = std::unordered_map<std::string, std::string>; // Podría ser un struct con más datos

    // Modelos de voz
    TTSModelsMap            loaded_models_;         // Mapa de modelos TTS cargados
    short                   num_available_models_;  // Número de modelos disponibles
    size_t                  num_threads_;           // Número de hilos con los que se generarán los audios
    bool                    concurrent_init_;       // Activa/desactiva la inicialización concurrente (experimental)
    std::string const       models_path_;           // Ruta de carpetas donde residen los modelos
    mutable std::mutex      models_mutex_;          // Mutex para proteger el mapa de modelos y init_percent

    // Datos de modelos
    TTSTextsMap             processing_texts_;      // Relaciona modelo - texto que está procesando
    mutable std::mutex      processing_mtx_;        // Mutex para proteger el acceso al mapa (mutable para métodos const)

    // Datos del modulo TTS
    std::atomic<short>      active_tasks_;      // Indica si hay algo en ejecución
    unsigned short          num_load_retries_;  // Número máximo de reintentos para recargar modelos que fallaron al cargar
    std::atomic<bool>       running_;           // Indica si no se ha comandado destruir la clase
    std::atomic<bool>       loading_;           // Indica si está cargando modelos
    std::mutex              exit_mtx_;          // Evita destruir TTSMgr si hay algo ejecutándose
    std::condition_variable exit_cv_;           // Notifica cuándo paran las tareas
};
