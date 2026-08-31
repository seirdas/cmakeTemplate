#pragma once

#include "app/IModule.hpp"
#include <memory>


// Foward declaration
class IAppControl;
struct TTSPacket;


/** 
 * @class TTSDispatcher
 * @brief Clase de lógica de reproducción de tonos a partir de un paquete
 *  de datos externo (de red, o de servidor iComm)
 */
class TTSDispatcher : public IModule {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    TTSDispatcher();
    
    /**
     * @brief Destructor
     */
    ~TTSDispatcher();

    // Sin copia ni movimiento
    TTSDispatcher(const TTSDispatcher&) = delete;
    TTSDispatcher& operator=(const TTSDispatcher&) = delete;
    TTSDispatcher(TTSDispatcher&&) = delete;
    TTSDispatcher& operator=(TTSDispatcher&&) = delete;


// Métodos comunes de módulo (IModule) --------------------------------------------------

    /**
     * @brief Inicializa la lógica de reproducción de tonos
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true si la inicialización fue exitosa, @c false si hubo algún error 
     */
    bool init(void* config) override;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config) override;

    /**
     * @brief Cierra la lógica y libera los recursos asociados.
     * @return @c true Si ha cerrado correctamente, @c false en caso de error
     */
    bool close() override;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Toma un paquete de datos de TTS y lo gestiona para reproducirlo
     * @details Esta función es llamada externamente por quien le mande el paquete.
     *  (Habitualmente por el iComm) 
     * @return 
     */
    bool Dispatch(TTSPacket* data);


private:


/************ Variables ********************************************************/

// Datos de paquetes gestionados
    unsigned long   last_packet_hash_;  ///< Hash del último data recibido, para comparar duplicados

};





/* LOGICA ANTIGUA TTSMgr */

    //std::unique_lock<std::mutex> lock(queue_mtx_);

    // // Comprobar si la entidad existe ya en la info de algún player
    // TTSMgrInfo* myinfo = nullptr;
    // for (auto& it : ttsPlayers_) {
    //     for (auto& info : PlayersInfo_[it.first])
    //         if(entityName == info.entityName) {
    //             myinfo = &info;
    //             break;
    //         }
    //     if (myinfo) break;
    // }

    // // Si se ha definido un nuevo modelo de voz, se asigna (exista o no la info)
    // if (!modelName.empty()) {
    //     if (!ttsCore_.isModelLoaded(modelName)) {
    //         SYS_WARN("TTSMgr","Play error: Model selected doesn't exist");
    //         return false;
    //     }
    //     /* #TODO */
    //     //myinfo->model_name_assigned = modelName;
    // }

    // // Caso cuando no existe la info (info nueva)
    // if (!myinfo) {
    //     TTSMgrInfo newInfo;
    //     newInfo.entityName = entityName;

    //     // Si no se ha definido un modelo de voz, elegir uno cualquiera
    //     if (modelName.empty()) {
    //         std::vector<std::string> models = ttsCore_.getLoadedModels();
    //         if (models.empty()) {
    //             SYS_WARN("TTSMgr","Play error: Cannot gather any TTS model.");
    //             return false;
    //         }
    //         unsigned short sel = rand() % models.size();
    //         /* #TODO */
    //         //myinfo->model_name_assigned = modelName;
    //     }

    //     // Guardar la entidad en la info
    //     newInfo.entityName = entityName;
    // }

    // Desbloqueo de mutex antes de reproducir para evitar deadlocks
    //lock.unlock();
    
    // Asignar a un PlayerTTS...
    /* #TODO */

    // Callback para mostrar texto mientras se está reproduciendo, quitar después
    /* #TODO */



    // /**
    //  * @brief Estructura de información de elementos en reproducción
    //  * @details Contiene además los datos de TTSPacket recibidos 
    //  */
    // struct TTSMgrInfo : TTSPacket {
    //     std::chrono::seconds    keep_alive_seconds_;    ///< Tiempo de vida de la asignación voz <-> entidad
    //     std::string             model_name_assigned;    ///< Nombre del modelo asociado a la entidad
    // };
    // using TTSInfos      = std::unordered_map<std::string, std::vector<TTSMgrInfo>>;
