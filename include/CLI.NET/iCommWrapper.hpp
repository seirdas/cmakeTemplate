#pragma once

#using <system.dll>
#using <iComm.dll>
#using <iComm.iATC.dll>


// Declaración implícita
class TTSMgr;


/**
 * @class iCommWrapper
 * @brief Managed class (C++/CLI) for .NET iComm dll
 *   Clase para compatibilidad con .NET de iComm
 *  especial para manejar los eventos y las funciones delegadas de la librería iComm
 *   Compatibilidad con iComm de .NET, clase autogestionada (managed) con el _ref class_
 *   Administrada con gc (garbage collector)
 *   Le pasa los datos necesarios a la logica del TTS.
 */
public ref class iCommWrapper {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     *  Inicializa el módulo TTSMgr y obtiene instancia de iComm
     */
    iCommWrapper(TTSMgr* tts);

    /**
     * @brief Destructor.
     *  Libera memoria del puntero inicializado a iComm
     */
    ~iCommWrapper();


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


// Notify functions (to use externally) -------------------------------------------------

    /**
     * @brief Notifica al iComm que se ha procesado el mensaje
     * @param MsgID 
     * @param LocalID 
     */
    void notifyFinished(unsigned int MsgID, unsigned int LocalID);

private:

// Delegate functions - Wrappers for native callbacks -----------------------------------

    /**
     * @brief Función registrada para cuando se ha conectado
     * @param _pConnectionEventArgs 
     */
    void OnConnected_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs);

    /**
     * @brief LLAMADO POR ICOMM - Función registrada para cuando llega información determinada
     * @param _pConnectionEventArgs 
     */
    void OnInfoConnection_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs);

    /**
     * @brief Función registrada para cuando llega información para DACS (coberturas, radios, etc.)
     * @param _pNetData 
     */
    void OnReceivedINFO_DACS_Wrapper(iComm::Net::Data::NetData^ _pNetData);

    /**
     * @brief Función registrada para cuando llega Texto,ID,etc. para DACS
     * @param _pNetData 
     */
    void OnReceivedTEXT_VOICE_COMMAND_Wrapper(iComm::Net::Data::NetData^ _pNetData);

    
/************ Variables ********************************************************/

// Inicialización y ejecución
    bool        initialized_;   ///< Bandera para indicar inicialización exitosa

// Datos
    long long   ATIS_ID_;        ///< Identificador de ATIS
    long long   ATC_ID_;         ///< Identificador de ATC

// Módulos
    iComm::IMessageSender^  iCommMgr;   ///< icomm manager pointer instance (.NET)
    TTSMgr*                 tts_;       ///< Puntero a TTSMgr para usar sus funciones

};
