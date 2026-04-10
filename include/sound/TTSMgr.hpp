#pragma once

#include "sherpa-onnx/c-api/c-api.h"
#include <iostream>
#include <thread>               // num_threads_

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
     * @brief Inicializa el módulo TTS cargando los modelos de voz.
     * @return true si la inicialización fue exitosa, false en caso de error.
     */
    bool init();

    /**
     * @brief Libera los recursos asociados a los modelos de voz cargados.
     */
    void cerrar();

    /**
     * @brief Genera un audio a partir de un texto usando el modelo de voz especificado.
     * @param text El texto a convertir en audio.
     * @param wavname El nombre del archivo WAV de salida (incluyendo la extensión .wav).
     * @return true si la generación fue exitosa, false en caso de error.
     */
    bool generate(std::string text, std::string wavname);

    /**
     * @brief Obtiene el porcentaje de inicialización del módulo TTS.
     * @return El porcentaje de inicialización (0 a 100).
     */
    short getInitPercent();

private:
    using TTSModelsMap = std::unordered_map<std::string, const SherpaOnnxOfflineTts*>;

    TTSModelsMap        tts_models_;        // TTS configurado con una voz
    int32_t             num_threads_;       // Número de hilos con los que se generarán los audios
    short               init_percent_;      // Porcentaje de inicialización (100 = full init)
    
    std::string const   models_path_    = "./tts-voices/";  // Ruta de carpetas donde residen los modelos
};
