#include "CLI.NET/iCommMgr.hpp"
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
#include "datatypes/TTSDataTypes.hpp"

#include "dispatchers/TTSDispatcher.hpp"


// General ------------------------------------------------------------------------------

iCommMgr::iCommMgr(TTSDispatcher* tts) :
    tts_(tts),
    icomm(iComm::iCommManager::GetInstance()),
    initialized_(false),
    ATIS_ID_(0),
    ATC_ID_(0)
{

};

iCommMgr::~iCommMgr() {
    close();

    // Borrar puntero a iCommMgr
    if (icomm != nullptr) {
        delete icomm; 
        icomm = nullptr;
    }
}


// Ejecución ----------------------------------------------------------------------------

bool iCommMgr::init() {

    // Obtener parámetros de configuración de iCommConfigFile.xml
    SYS_INFO("iCommMgr","Parsing iCommConfigFile.xml...");
	icomm->ParseConfigFile("iCommConfigFile.xml");

	// recoge el ID de ATIS y el ID de ATC a partir del localID de las conexiones activas del xml de config del iComm
    SYS_INFO("iCommMgr","Gathering ATIS/ATC IDs...");
    std::string name;
    for each (iComm::Net::Config::ConnectionConfigData^ connection in icomm->GetLocalConnectionsInfo()) {
        // Comprueba si existe el nombre
        if(!connection->Name) {
            SYS_WARN("iCommMgr","iComm fail: connection->Name empty");
            return false;
        }

        // Obtener el ID de ATIS/ATC a partir del nombre de conexión
		name = msclr::interop::marshal_as<std::string>(connection->Name);

        // Buscarlo en minúsculas (case insensitive)
		for (char &c : name)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); 

        // Búsqueda de ID
		if (name.find("atis") != std::string::npos)
            ATIS_ID_ = connection->LocalID;
        if (name.find("atc") != std::string::npos)
            ATC_ID_ = connection->LocalID;
	}

	/* Inicialización de Funciones Delegadas (callbacks) del iComm. */

    // Función registrada para cuando se establece la conexión
    SYS_INFO("iCommMgr","Subscribing onConnected event...");
	icomm->AddEventConnected(
		gcnew iComm::iCommManager::DelegateOnConnected(this, &iCommMgr::on_connected)
    );

    // Función registrada para cuando llega información de conexión
    SYS_INFO("iCommMgr","Subscribing OnInfoConnection event...");
	icomm->AddEventInfoStatusConnection(
		gcnew iComm::iCommManager::DelegateOnInfoConnection(this, &iCommMgr::on_info_connection)
    );

	/*	Salta a la funcion OnReceivedINFO_DACS_Wrapper cuando recibe un mensaje de iComm
	*	Siendo:
	*	 iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	 iComm::iATC::Identifiers::MessageID::MSG_INFO_DACS	== El NetData es de tipo MSG_INFO_DACS (del iComm.iATC.dll)
    *   aquí el iComm::Net::Data::NetData -> iComm::Net::Data::DataDACS
	*/
    SYS_INFO("iCommMgr","Subscribing OnReceivedINFO_DACS event...");
	icomm->AddDelegateToMessage(
        iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_INFO_DACS,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommMgr::on_received_info_dacs)
    );

	// Datos Delegados (callbacks) de datos.

	/*	Salta a la funcion on_received_text_voice_command cuando recibe un mensaje de iComm 
	*	Siendo:
	*	 iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	 iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND	== El NetData es de tipo DataTextVoiceCommand (del iComm.iATC.dll)
    *   aquí el iComm::Net::Data::NetData -> iComm::Net::Data::DataTextVoiceCommand
	*/
    SYS_INFO("iCommMgr","Subscribing OnReceivedTEXT_VOICE_COMMAND event...");
	icomm->AddDelegateToMessage(
		iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommMgr::on_received_text_voice_command)
    );


    SYS_INFO("iCommMgr","Starting iComm Client...");
    icomm->Start();

    
    SYS_INFO("iCommMgr","iComm initializing OK");
    initialized_ = true;
    return initialized_;       // <- true
}

bool iCommMgr::close() {
    if(icomm->Active) 
        icomm->Stop();

    return !icomm->Active;
}

bool iCommMgr::isInitialized() {
    return initialized_;
}

bool iCommMgr::isRunning() {
	return icomm->Active;
}


// Información --------------------------------------------------------------------------

unsigned short iCommMgr::get_ATIS_ID() { 
    return ATIS_ID_; 
}

unsigned short iCommMgr::get_ATC_ID() { 
    return ATC_ID_; 
}


// Notify functions (to use externally) -------------------------------------------------

void iCommMgr::notifyFinished(unsigned int MsgID, unsigned int LocalID) {

	iComm::iATC::DataRadioMsgStatus^ dataRadioMsgStatus = gcnew iComm::iATC::DataRadioMsgStatus();
	dataRadioMsgStatus->MsgID = MsgID;
	dataRadioMsgStatus->Status = iComm::iATC::DataRadioMsgStatus::EStatus::FINISHED;

	icomm->SendMessage(LocalID, dataRadioMsgStatus, true, false);
}


// Delegate functions (iComm Callbacks) -------------------------------------------------

void iCommMgr::on_connected(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
	/* #TODO */

    SYS_INFO("iCommMgr","OnConnected callback called");

    unsigned short _IConnectionID = 10;
    short _IMsgID = -1;

	// iCommMgr->SubscribeMsg(true, _IConnectionID, iComm::iATC::Identifiers::FACTORY_NAME, _iMsgID);
	// tts->OnConnected(_pConnectionEventArgs);
}

void iCommMgr::on_info_connection(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
    /* #TODO */
    
    /* No lo pongo porque salta cada sec aprox. */
    //SYS_INFO("iCommMgr","OnInfoConnection callback called");

	// tts->OnInfoConnection(_pConnectionEventArgs);
}

void iCommMgr::on_received_info_dacs(iComm::Net::Data::NetData^ _pNetData) {
    /* #TODO */
    SYS_INFO("iCommMgr","on_received_info_dacs callback called");

	// tts->OnReceivedINFO_DACS(static_cast<iComm::iATC::DataDACS^>(_pNetData));
}

void iCommMgr::on_received_text_voice_command(iComm::Net::Data::NetData^ _pNetData) {

    SYS_INFO("iCommMgr","on_received_text_voice_command callback called");

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
	data.entityName = get_entity(packet);        // Sustituye al cálculo de la voz
    data.texto      = converter.to_bytes(msclr::interop::marshal_as<std::wstring>(packet->VoiceText));
    data.lang       = get_language(packet);

    SYS_WARN("iCommMgr","on_received_text_voice_command not implemented");
	//tts_->play(data);
}


// Funciones auxiliares -----------------------------------------------------------------

std::string iCommMgr::get_entity(iComm::iATC::DataTextVoiceCommand^ packet) {
    
    // debería ser una voz de mujer (podría ser la misma siempre)
    if (packet->NetCreator->ConnectionConfigData->LocalID == ATIS_ID_)
        return "ATIS";

    if (packet->NetCreator->ConnectionConfigData->LocalID == ATC_ID_) {
        if (packet->Sender == iComm::iATC::Identifiers::SenderReceiverType::IA_PILOT)
            return "PILOT";     // (Los pilotos suelen ser hombres)
        else {
            switch (packet->ControllerType) {
            case iComm::iATC::Identifiers::ControllerType::GROUND:
                return "CONTROLER_GROUND";
            case iComm::iATC::Identifiers::ControllerType::TOWER:
                return "CONTROLER_TOWER";
            case iComm::iATC::Identifiers::ControllerType::APPROACH:
                return "CONTROLER_APPROACH";
            }
        }
    }

    return "OTHER";		// voz por defecto si no se cumplen las condiciones anteriores
}

std::string iCommMgr::get_language(iComm::iATC::DataTextVoiceCommand^ packet) {
    switch (packet->Language) {
        case iComm::iATC::Identifiers::Language::ENGLISH:
            return "en";
        // case iComm::iATC::Identifiers::Language::ENGLISH_GB:
        //     return "en_GB";
        // case iComm::iATC::Identifiers::Language::ENGLISH_USA:
        //     return "en_US";
        case iComm::iATC::Identifiers::Language::SPANISH:
            return "es";
        default:
            return "en";       // Inglés por defecto
    }
}
