#include "system/RegMgr.hpp"

#define _HKEY_ HKEY_CURRENT_USER


/*************************************/
// Implementación de RegMgr
/*************************************/

RegMgr::RegMgr()
{

}

RegMgr::~RegMgr()
{}

// Getters ______________________________________________________________________________

bool RegMgr::Get_REG(std::string path, std::string clave, uint32_t *dwvalor, uint64_t *qwvalor, std::string *csvalor)
{
	HKEY reg_key;
	DWORD tamBuffer=0;
	BYTE Buffer[500];
	DWORD Type=0;
	tamBuffer=500;
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_READ,&reg_key) != ERROR_SUCCESS) return false;
	if(RegQueryValueEx(reg_key,clave.c_str(),NULL,&Type,Buffer,&tamBuffer) != ERROR_SUCCESS) return false;
	if (Type==REG_DWORD) memcpy(dwvalor,Buffer,tamBuffer);
	if (Type==REG_DWORD_LITTLE_ENDIAN) memcpy(dwvalor,Buffer,tamBuffer);
	if (Type==REG_QWORD) memcpy(qwvalor,Buffer,tamBuffer);
	if (Type==REG_QWORD_LITTLE_ENDIAN) memcpy(qwvalor,Buffer,tamBuffer);
	if (Type==REG_SZ) *csvalor=(TCHAR*)Buffer;
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;
	return true;
}

bool RegMgr::Get_DWORD(std::string path, std::string clave, uint32_t *valor)
{
	HKEY reg_key;
	DWORD tamBuffer;
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_READ,&reg_key)
		!= ERROR_SUCCESS) return false;
	if(RegQueryValueEx(reg_key,clave.c_str(),NULL,NULL,NULL,&tamBuffer)
		!= ERROR_SUCCESS) return false;
	if(RegQueryValueEx(reg_key,clave.c_str(),NULL,NULL,(LPBYTE)valor,&tamBuffer)
		!= ERROR_SUCCESS) return false;
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;
	return true;
}

bool RegMgr::Get_STR(std::string path, std::string clave, std::string *valor)
{
	HKEY reg_key;
	DWORD tamBuffer = 500;
	TCHAR v[250];
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_READ,&reg_key)
		!= ERROR_SUCCESS) return false;
	if(RegQueryValueEx(reg_key,clave.c_str(),NULL,NULL,(LPBYTE)v,&tamBuffer)
		!= ERROR_SUCCESS) return false;
	*valor=v;
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;
	return true;
}

// Setters ______________________________________________________________________________

bool RegMgr::Set_DWORD(std::string path, std::string clave, uint32_t valor)
{
	HKEY reg_key;
	DWORD tamBuffer;
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_WRITE,&reg_key)
		!= ERROR_SUCCESS) return false;
	tamBuffer = sizeof(DWORD);
	if(RegSetValueEx(reg_key,clave.c_str(),0,REG_DWORD,(LPBYTE)&valor,tamBuffer)
		!=ERROR_SUCCESS)return false; 
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;
	return true;

}

bool RegMgr::Set_REG(std::string path, std::string clave, uint32_t *dwvalor, uint64_t *qwvalor, std::string *csvalor)
{
    HKEY reg_key;
    DWORD Type = 0;
    const BYTE* pData = nullptr;
    DWORD tamBuffer = 0;

    if (dwvalor != nullptr) {
        Type = REG_DWORD;
        tamBuffer = sizeof(uint32_t);
        pData = reinterpret_cast<const BYTE*>(dwvalor);
    }
    else if (qwvalor != nullptr) {
        Type = REG_QWORD;
        tamBuffer = sizeof(uint64_t);
        pData = reinterpret_cast<const BYTE*>(qwvalor);
    }
    else if (csvalor != nullptr) {
        Type = REG_SZ;
        // c_str() incluye el terminador nulo, size() no. 
        // Para el registro, usualmente se incluye el +1.
        tamBuffer = static_cast<DWORD>(csvalor->size() + 1);
        pData = reinterpret_cast<const BYTE*>(csvalor->c_str());
    }

    if (pData == nullptr) return false;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, path.c_str(), 0, KEY_WRITE, &reg_key) != ERROR_SUCCESS) 
        return false;

    bool success = (RegSetValueExA(reg_key, clave.c_str(), 0, Type, pData, tamBuffer) == ERROR_SUCCESS);
    
    RegCloseKey(reg_key);
    return success;
}

bool RegMgr::Set_STR(std::string path, std::string clave, std::string valor)
{
	HKEY reg_key;
	DWORD tamBuffer = 500;
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_WRITE,&reg_key)
		!= ERROR_SUCCESS) return false;
	if (RegSetValueEx(reg_key, clave.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(valor.c_str()), (DWORD)(valor.length() + 1)) != ERROR_SUCCESS) 
    	return false;
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;
	return true;

}

bool RegMgr::WaitUntilChange(std::string path)
{
	HKEY reg_key;
	if(RegOpenKeyEx(_HKEY_,path.c_str(),0,KEY_NOTIFY,&reg_key) != ERROR_SUCCESS) return false;
	if(RegNotifyChangeKeyValue(reg_key,TRUE,REG_NOTIFY_CHANGE_LAST_SET,NULL,FALSE) != ERROR_SUCCESS) return false;
	if(RegCloseKey(reg_key)!=ERROR_SUCCESS) return false;

	return true;
}