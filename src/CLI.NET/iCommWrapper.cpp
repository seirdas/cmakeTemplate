#include "CLI.NET/iCommWrapper.hpp"
#include "system/SystemMgr.hpp"
#include <msclr/marshal_cppstd.h>
#include <algorithm>
#include <cctype>
#include <string>


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

	// Datos Delegados (callbacks) de conexión.
    SYS_INFO("iCommWrapper","Subscribing onConnected event...");
	iCommMgr->AddEventConnected(
		gcnew iComm::iCommManager::DelegateOnConnected(this, &iCommWrapper::OnConnected_Wrapper));

    /* No existe esta función en el iComm */
    SYS_INFO("iCommWrapper","Subscribing OnInfoConnection event...");
	iCommMgr->AddEventInfoStatusConnection(
		gcnew iComm::iCommManager::DelegateOnInfoConnection(this, &iCommWrapper::OnInfoConnection_Wrapper));

	// Datos Delegados (callbacks) de datos.

	/*	Salta a la funcion OnReceivedTEXT_VOICE_COMMAND_Wrapper cuando recibe un mensaje de iComm 
	*	Siendo:
	*	iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND	== El mensaje ^NetData es de tipo DataTextVoiceCommand (del iComm.iATC.dll)
	*/
    SYS_INFO("iCommWrapper","Subscribing OnReceivedTEXT_VOICE_COMMAND event...");
	iCommMgr->AddDelegateToMessage(
		iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommWrapper::OnReceivedTEXT_VOICE_COMMAND_Wrapper));

	/*	Salta a la funcion OnReceivedINFO_DACS_Wrapper cuando recibe un mensaje de iComm
	*	Siendo:
	*	iComm::iATC::Identifiers::FACTORY_NAME == iComm.iATC	(del iComm.iATC.dll)
	*	iComm::iATC::Identifiers::MessageID::MSG_TEXT_VOICE_COMMAND	== El mensaje ^NetData es de tipo MSG_INFO_DACS (del iComm.iATC.dll)
	*/
    SYS_INFO("iCommWrapper","Subscribing OnReceivedINFO_DACS event...");
	iCommMgr->AddDelegateToMessage(iComm::iATC::Identifiers::FACTORY_NAME,
		(int)iComm::iATC::Identifiers::MessageID::MSG_INFO_DACS,
		gcnew iComm::iCommManager::DelegateOnNetMessage(this, &iCommWrapper::OnReceivedINFO_DACS_Wrapper));

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


// Delegate functions - Wrappers for native callbacks -----------------------------------

void iCommWrapper::OnConnected_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
	/* #TODO */
	//iCommMgr->SubscribeMsg(true, _IConnectionID, iComm::iATC::Identifiers::FACTORY_NAME, _iMsgID);
	//tts->OnConnected(_pConnectionEventArgs);
}

void iCommWrapper::OnInfoConnection_Wrapper(iComm::Net::ConnectionEventArgs^ _pConnectionEventArgs) {
    /* #TODO */
	//tts->OnInfoConnection(_pConnectionEventArgs);
}

void iCommWrapper::OnReceivedINFO_DACS_Wrapper(iComm::Net::Data::NetData^ _pNetData) {
    /* #TODO */
	//tts->OnReceivedINFO_DACS(static_cast<iComm::iATC::DataDACS^>(_pNetData));
}

void iCommWrapper::OnReceivedTEXT_VOICE_COMMAND_Wrapper(iComm::Net::Data::NetData^ _pNetData) {
    /* #TODO */
	//tts->OnReceivedTEXT_VOICE_COMMAND(static_cast<iComm::iATC::DataTextVoiceCommand^>(_pNetData));
}


// Notify functions (to use externally) -------------------------------------------------

void iCommWrapper::notifyFinished(unsigned int MsgID, unsigned int LocalID) {

	iComm::iATC::DataRadioMsgStatus^ dataRadioMsgStatus = gcnew iComm::iATC::DataRadioMsgStatus();
	dataRadioMsgStatus->MsgID = MsgID;
	dataRadioMsgStatus->Status = iComm::iATC::DataRadioMsgStatus::EStatus::FINISHED;

	iCommMgr->SendMessage(LocalID, dataRadioMsgStatus, true, false);
}
