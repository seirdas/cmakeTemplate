#pragma once

#using <system.dll>
#using <iComm.dll>
#using <iComm.iATC.dll>

#include <string>


// Declaración implícita
class TTSDispatcher;


/**
 * @class iCommMgr
 * @brief Managed class (C++/CLI) for .NET iComm dll
 *   Clase para compatibilidad con .NET de iComm
 *  especial para manejar los eventos y las funciones delegadas de la librería iComm
 *   Compatibilidad con iComm de .NET, clase autogestionada (managed) con el _ref class_
 *   Administrada con gc (garbage collector)
 *   Le pasa los datos necesarios a la logica del TTS.
 */
public ref class iCommMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor.
     */
    iCommMgr();

    /**
     * @brief Destructor.
     *  Libera memoria del puntero inicializado a iComm
     */
    ~iCommMgr();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa el cliente iComm.
     *  - Lee el archivo de configuración de iComm (iCommConfigFile)
     *  - Obtiene los identificadores de ATIS/ATC
     *  - Suscribe eventos a funciones que ejecutará TTSMgr
     * @return @c true Si la inicialización ha sido correcta, @c false en caso contrario
     */
    bool init();

    /**
     * @brief Para la conexión y las suscripciones del iComm
     *  y borra el puntero a la instancia iCommMgr
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close();

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @note En esta clase CLI.NET, no se puede poner const
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized();

    /**
     * @brief Devuelve si la conexión de iComm está activa
     * @return @c true Si está activa, @c false en caso contrario
     */
    bool isRunning();


// Información --------------------------------------------------------------------------

    /**
     * @brief Devuelve el identificador de ATIS
     * @return Número identificador ID de ATIS
     */
    unsigned short get_ATIS_ID();
    
    /**
     * @brief Devuelve el identificador de ATC
     * @return Número identificador ID de ATC
     */
    unsigned short get_ATC_ID();


// Dispatch -----------------------------------------------------------------------------

    /* (#TBD) Inyectar el callback para dispatch los datos que le llegan */
    // void setCallback_onReceive(std::function<?????( ?? )> cb);


// Notify functions (to use externally) -------------------------------------------------

    /**
     * @brief Notifica al iComm que se ha procesado el mensaje
     * @param MsgID 
     * @param LocalID 
     */
    void notifyFinished(unsigned int MsgID, unsigned int LocalID);


private:

// Delegate functions (iComm Callbacks) -------------------------------------------------

    /**
     * @brief Función registrada para cuando se ha conectado
     * @param _pConnectionEventArgs 
     */
    void on_connected(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs);

    /**
     * @brief LLAMADO POR ICOMM - Función registrada para cuando llega información determinada
     * @param _pConnectionEventArgs 
     */
    void on_info_connection(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs);

    /**
     * @brief Función registrada para cuando llega información para DACS (coberturas, radios, etc.)
     * @param _pNetData 
     */
    void on_received_info_dacs(iComm::Net::Data::NetData^ _pNetData);

    /**
     * @brief Función registrada para cuando llega Texto,ID,etc. para DACS
     * @param _pNetData 
     */
    void on_received_text_voice_command(iComm::Net::Data::NetData^ _pNetData);


// Funciones auxiliares -----------------------------------------------------------------

    /**
     * @brief Devuelve el nombre de entidad en función de parámetros del iComm
     * @return Nombre de entidad
     */
    std::string get_entity(iComm::iATC::DataTextVoiceCommand^ packet);

    /**
     * @brief Devuelve el idioma de entidad en función de parámetros del iComm
     * @return Idioma de entidad
     */
    std::string get_language(iComm::iATC::DataTextVoiceCommand^ packet);
   

/************ Variables ********************************************************/

// Inicialización y ejecución
    bool        initialized_;   ///< Bandera para indicar inicialización exitosa

// Datos
    long long   ATIS_ID_;        ///< Identificador de ATIS
    long long   ATC_ID_;         ///< Identificador de ATC

// Módulos
    iComm::IMessageSender^  icomm;   ///< icomm manager pointer instance (.NET)

};
