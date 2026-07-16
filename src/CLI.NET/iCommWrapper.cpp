#include "CLI.NET/iCommWrapper.hpp"
#include "system/SystemMgr.hpp"
#include <msclr/marshal_cppstd.h>
#include <algorithm>
#include <cctype>
#include <string>

// Transformación de wstring a string
#include <locale>
#include <codecvt>
#include <cwctype>

// Datos para el paquete al TTS
#include "tts/TTSDataTypes.hpp"


// General ------------------------------------------------------------------------------

iCommWrapper::iCommWrapper(TTSMgr* tts) :
    tts_(tts),
    iCommMgr(iComm::iCommManager::GetInstance()),
    initialized_(false),
    ATIS_ID_(0),
    ATC_ID_(0)
{

};

iCommWrapper::~iCommWrapper() {
    close();

    // Borrar puntero a iCommMgr
    if (iCommMgr != nullptr) {
        delete iCommMgr; 
        iCommMgr = nullptr;
    }
}


// Ejecución ----------------------------------------------------------------------------

bool iCommWrapper::init() {

    // Obtener parámetros de configuración de iCommConfigFile.xml
    SYS_INFO("iCommWrapper","Parsing iCommConfigFile.xml...");
	iCommMgr->ParseConfigFile("iCommConfigFile.xml");

	// recoge el ID de ATIS y el ID de ATC a partir del localID de las conexiones activas del xml de config del iComm
    SYS_INFO("iCommWrapper","Gathering ATIS/ATC IDs...");
    std::string name;
    for each (iComm::Net::Config::ConnectionConfigData^ connection in iCommMgr->GetLocalConnectionsInfo()) {
        // Comprueba si existe el nombre
        if(!connection->Name) {
            SYS_WARN("iCommWrapper","iComm fail: connection->Name empty");
            return false;
        }

        // Obtener el ID de ATIS/ATC a partir del nombre de conexión
		name = msclr::interop::marshal_as<std::string>(connection->Name);

        // Buscarlo en minúsculas (case insensitive)
		for (char &c : name)
            c = std::tolower(static_cast<unsigned char>(c));

        // Búsqueda de ID
		if (name.find("atis") != std::string::npos)
            ATIS_ID_ = connection->LocalID;
        if (name.find("atc") != std::string::npos)
            ATC_ID_ = connection->LocalID;
	}

	/* Inicialización de Funciones Delegadas (callbacks) del iComm. */

    // Función registrada para cuando se establece la conexión
    SYS_INFO("iCommWrapper","Subscribing onConnected event...");
	iCommMgr->AddEventConnected(
		gcnew iComm::iCommManager::DelegateOnConnected(this, &iCommWrapper::OnConnected_Wrapper)
    );

    // Función registrada para cuando llega información de conexión
    SYS_INFO("iCommWrapper","Subscribing OnInfoConnection event...");
	iCommMgr->AddEventInfoStatusConnection(
		gcnew iComm::iCommManager::DelegateOnInfoConnection(this, &iCommWrapper::OnInfoConnection_Wrapper)
    );

	/*	Salta a la funcion OnReceivedINFO_DACS_Wrapper cuando recibe un mensaje de iComm
	*	Siendo:
	*	 iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	 iComm::iATC::Identifiers::MessageID::MSG_INFO_DACS	== El NetData es de tipo MSG_INFO_DACS (del iComm.iATC.dll)
    *   aquí el iComm::Net::Data::NetData -> iComm::Net::Data::DataDACS
	*/
    SYS_INFO("iCommWrapper","Subscribing OnReceivedINFO_DACS event...");
	iCommMgr->AddDelegateToMessage(
        iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_INFO_DACS,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommWrapper::OnReceived_InfoDacs)
    );

	// Datos Delegados (callbacks) de datos.

	/*	Salta a la funcion OnReceived_TextVoiceCommand cuando recibe un mensaje de iComm 
	*	Siendo:
	*	 iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	 iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND	== El NetData es de tipo DataTextVoiceCommand (del iComm.iATC.dll)
    *   aquí el iComm::Net::Data::NetData -> iComm::Net::Data::DataTextVoiceCommand
	*/
    SYS_INFO("iCommWrapper","Subscribing OnReceivedTEXT_VOICE_COMMAND event...");
	iCommMgr->AddDelegateToMessage(
		iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommWrapper::OnReceived_TextVoiceCommand)
    );

	/*	Salta a la funcion OnReceived_TextVoiceCommand cuando recibe un mensaje de iComm 
	*	Siendo:
	*	iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND	== El mensaje ^NetData es de tipo DataTextVoiceCommand (del iComm.iATC.dll)
	*/
    // aquí el iComm::Net::Data::NetData -> iComm::Net::Data::DataVoiceMessageToTTS
    SYS_INFO("iCommWrapper","Subscribing OnReceivedINFO_DACS event...");
	iCommMgr->AddDelegateToMessage(
        iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_VOICE_MESSAGE_TO_TTS,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommWrapper::OnReceived_OldTextVoiceCommand)
    );


    SYS_INFO("iCommWrapper","Starting iComm Client...");
    iCommMgr->Start();

    
    SYS_INFO("iCommWrapper","Initializing OK");
    initialized_ = true;
    return initialized_;       // <- true
}

bool iCommWrapper::close() {
    if(iCommMgr->Active) 
        iCommMgr->Stop();

    return !iCommMgr->Active;
}

bool iCommWrapper::isInitialized() {
    return initialized_;
}

bool iCommWrapper::isRunning() {
	return iCommMgr->Active;
}


// Información --------------------------------------------------------------------------

unsigned short iCommWrapper::get_ATIS_ID() { 
    return ATIS_ID_; 
}

unsigned short iCommWrapper::get_ATC_ID() { 
    return ATC_ID_; 
}


// Notify functions (to use externally) -------------------------------------------------

void iCommWrapper::notifyFinished(unsigned int MsgID, unsigned int LocalID) {

	iComm::iATC::DataRadioMsgStatus^ dataRadioMsgStatus = gcnew iComm::iATC::DataRadioMsgStatus();
	dataRadioMsgStatus->MsgID = MsgID;
	dataRadioMsgStatus->Status = iComm::iATC::DataRadioMsgStatus::EStatus::FINISHED;

	iCommMgr->SendMessage(LocalID, dataRadioMsgStatus, true, false);
}


// Delegate functions (iComm Callbacks) -------------------------------------------------

void iCommWrapper::OnConnected_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
	/* #TODO */

    SYS_INFO("iCommWrapper","OnConnected callback called");

    unsigned short _IConnectionID = 10;
    short _IMsgID = -1;

	// iCommMgr->SubscribeMsg(true, _IConnectionID, iComm::iATC::Identifiers::FACTORY_NAME, _iMsgID);
	// tts->OnConnected(_pConnectionEventArgs);
}

void iCommWrapper::OnInfoConnection_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
    /* #TODO */
    
    /* No lo pongo porque salta cada sec aprox. */
    //SYS_INFO("iCommWrapper","OnInfoConnection callback called");

	// tts->OnInfoConnection(_pConnectionEventArgs);
}

void iCommWrapper::OnReceived_InfoDacs(iComm::Net::Data::NetData^ _pNetData) {
    /* #TODO */
    SYS_INFO("iCommWrapper","OnReceived_InfoDacs callback called");

	// tts->OnReceivedINFO_DACS(static_cast<iComm::iATC::DataDACS^>(_pNetData));
}

void iCommWrapper::OnReceived_TextVoiceCommand(iComm::Net::Data::NetData^ _pNetData) {

    SYS_INFO("iCommWrapper","OnReceived_TextVoiceCommand callback called");

    // El paquete que llega aquí es de tipo DataDACS
	iComm::iATC::DataTextVoiceCommand^ packet = static_cast<iComm::iATC::DataTextVoiceCommand^>(_pNetData);

    // Conversor de wstring a string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    // Rellenar el paquete de datos del tts con la información que ha llegado del iComm
    TTSPacket data;

    // Datos del paquete
	data.LocalID	= packet->NetCreator->ConnectionConfigData->LocalID; // ATIS o ATC
	data.MsgID      = packet->RadioMsgID;
	data.ID_TX      = packet->SenderRemoteRadioID;
	data.entityID   = packet->SenderRadioID;
	data.entityName = getEntity(packet);        // Sustituye al cálculo de la voz
    data.texto      = converter.to_bytes(msclr::interop::marshal_as<std::wstring>(packet->VoiceText));
    data.lang       = "en-US";      // Inglés americano por defecto

    SYS_WARN("iCommWrapper","OnReceived_TextVoiceCommand not implemented");
	//tts_->play(data);
}

void iCommWrapper::OnReceived_OldTextVoiceCommand(iComm::Net::Data::NetData^ _pNetData) {
    
    SYS_INFO("iCommWrapper","OnReceived_OldTextVoiceCommand callback called");

    iComm::iATC::DataVoiceMessageToTTS^ packet = static_cast<iComm::iATC::DataVoiceMessageToTTS^>(_pNetData);
    
    // Sin usar, sin implementar
    SYS_WARN("iCommWrapper","OnReceived_OldTextVoiceCommand not implemented");

    
    return;
}


// Funciones auxiliares -----------------------------------------------------------------

std::string iCommWrapper::getEntity(iComm::iATC::DataTextVoiceCommand^ packet) {
    
    // debería ser una voz de mujer (podría ser la misma siempre)
    if (packet->NetCreator->ConnectionConfigData->LocalID == ATIS_ID_)
        return (packet->Language == iComm::iATC::Identifiers::Language::CHINESE) ? "VOICE_ATIS_CHINO" : "VOICE_ATIS";


    // (Los pilotos suelen ser hombres)

    if (packet->NetCreator->ConnectionConfigData->LocalID == ATC_ID_) {
        if (packet->Sender == iComm::iATC::Identifiers::SenderReceiverType::IA_PILOT)
            return "VOICE_PILOT";
        else {
            switch (packet->ControllerType) {
            case iComm::iATC::Identifiers::ControllerType::GROUND:
                return "VOICE_CONTROLER_GROUND";
            case iComm::iATC::Identifiers::ControllerType::TOWER:
                return "VOICE_CONTROLER_TOWER";
            case iComm::iATC::Identifiers::ControllerType::APPROACH:
                return "VOICE_CONTROLER_APPROACH";
            }
        }
    }

    return "VOICE_CONTROLER_CONTROL";		// voz por defecto si no se cumplen las condiciones anteriores
}