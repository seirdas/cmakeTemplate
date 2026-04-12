#pragma once

#include "sherpa-onnx/c-api/c-api.h"
#include <thread>               // num_threads_
#include <unordered_map>
#include <vector>
#include <condition_variable>
#include <filesystem>

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
     * @brief Genera un audio a partir de un texto usando el modelo de voz especificado.
     * @param text El texto a convertir en audio.
     * @param wavname El nombre del archivo WAV de salida (incluyendo la extensión wav).
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool generate(std::string const& text, std::string const& wavname);

    /**
     * @brief Obtiene el porcentaje de inicialización del módulo TTS.
     * @return El porcentaje de inicialización (0 a 100).
     */
    short getInitPercent() const;
  
    /**
     * @brief Obtiene una lista con los nombres de los modelos disponibles
     */
    std::vector<std::string> getAvailableModels();
      
    /**
     * @brief Obtiene una lista con los nombres de los modelos cargados
     */
    std::vector<std::string> getLoadedModels() const;

    /**
     * @brief Obtiene el número de modelos disponibles
     */
    short getAvailableNumModels() const;

    /**
     * @brief Obtiene el número de modelos cargados
     */
    short getLoadedNumModels() const;

    /**
     * @brief Obtiene la frecuencia de muestreo del modelo de voz
     */
    int getSampleRate(std::string const& modelName) const;

    /**
     * @brief Obtiene el número de speakers (hablantes) del modelo de voz
     */
    int getNumSpeakers(std::string const& modelName) const;


    /**
     * @brief Devuelve si el módulo tiene algún proceso activo.
     */
    bool isWorking() const;

private:

    /**
     * @brief Carga un modelo vits TTS 
     */
    bool load_vits_model(std::filesystem::path modelDir);

    /************ Variables ********************************************************/

    using TTSModelsMap = std::unordered_map<std::string, const SherpaOnnxOfflineTts*>;

    TTSModelsMap            loaded_models_;         // Mapa de modelos TTS cargados
    int32_t                 num_threads_;           // Número de hilos con los que se generarán los audios
    bool                    concurrent_init_;       // Activa/desactiva la inicialización concurrente (experimental)
    short                   init_percent_;          // Porcentaje de inicialización (100 = full init)
    std::string const       models_path_;           // Ruta de carpetas donde residen los modelos
    short                   num_available_models_;  // Número de modelos disponibles
    short                   num_loaded_models_;     // Número de modelos disponibles
    std::mutex              models_mutex_;          // Mutex para proteger el mapa de modelos y init_percent

    std::atomic<short>      active_tasks_;      // Indica si hay algo en ejecución
    std::atomic<bool>       running_;           // Indica si no se ha comandado destruir la clase
    std::mutex              exit_mtx_;          // Evita destruir TTSMgr si hay algo ejecutándose
    std::condition_variable exit_cv_;           // Notifica cuándo paran las tareas
};
