#pragma once


/**
 * @brief Estructura de datos para transferir a través de patrón observador
 */
struct OBS_SoundsData {

    // Dispositivos de captura/playback disponibles
    std::vector<std::string> available_captures;    ///< Dispositivos de captura disponibles en el sistema
    std::vector<std::string> available_playbacks;   ///< Dispositivos playback disponibles en el sistema

    // Modulos de audio existentes
    std::vector<std::string> captures;              ///< Lista de nombres de módulos de captura activos
    std::vector<std::string> players_audio;         ///< Lista de nombres de módulos reproductores de audio activos
    std::vector<std::string> players_tts;           ///< Lista de nombres de módulos reproductores de tts activos
    std::vector<std::string> players_morse;         ///< Lista de nombres de módulos reproductores de morse activos

    // Datos del tts
    struct TTS {
        unsigned short num_available_models;        ///< Número de modelos disponibles
        unsigned short num_loaded_models;           ///< Número de modelos cargados
        std::vector<std::string> available_models;  ///< Lista de nombres de modelos disponibles
        std::vector<std::string> loaded_models;     ///< Lista de nombres de modelos cargados
    }tts;                                           ///< Datos de TTS

};


// ######################################################################################


/**
 * @brief Clase con métodos de interfaz para patrón observador
 * @details Los métodos los llama el módulo (y le pasa sus los parámetros)
 *   y los ejecuta el observador
 * Son funciones para otras clases con los datos y el contexto de esta.
 */
class ISoundObserver {
public:

    /**
     * @brief Destructor de interfaz observer
     * @details Destructor virtual asegura que se ejecute primero 
     *  el destructor de la clase derivada y luego ésta
     * @details ``= default`` para implementación de destructor por defecto
     */
    virtual ~ISoundObserver() = default;


// Métodos que utilizan los observadores ------------------------------------------------
    
    /**
     * @brief Ejecuta acciones en función de los datos recibidos 
     *  por parámetro del TTS
     * @details Lo llama TTSMgr -> lo define GuiMgr lo que va a hacer
     * @param data 
     */
    virtual void onSoundsDataChanged(OBS_SoundsData const& data) = 0;
};
