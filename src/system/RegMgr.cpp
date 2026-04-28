#include "system/RegMgr.hpp"
#include "system/sys.hpp"

#ifdef _WIN32
// ============================================================
//  Windows
// ============================================================

	#include <windows.h>

	// General ------------------------------------------------------------------------------

	RegMgr::RegMgr() {

	}

	RegMgr::~RegMgr() {

	}

	// Getters ------------------------------------------------------------------------------

	uint32_t RegMgr::Get_DWORD(std::wstring const& root,std::wstring const& path, std::wstring const& clave) {

		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) {
			SYS_ERROR("RegMgr", "Root '" + toStr(root) + "' is not recognized or supported.");
			return 0;
		}

		// Comprueba si está sobreescribiendo una clave con otro tipo. AVISA PERO CONTINÚA
		DWORD actualType = (DWORD)queryType(ptr_Root, path, clave);
		if (actualType != REG_NONE && actualType != REG_DWORD)
			SYS_WARN("RegMgr", "Type mismatch at " + toStr(path) + ": expected " + 
					regTypeName(REG_DWORD) + " but found " + regTypeName(actualType));

		// Obtiene el valor
		uint32_t valor = 0;
		DWORD size = sizeof(uint32_t);
		LSTATUS status = RegGetValueW(
			static_cast<HKEY>(ptr_Root),
			path.c_str(),
			clave.c_str(),
			RRF_RT_REG_DWORD, // Solo acepta DWORD
			nullptr,
			&valor,
			&size
		);
		if (status != ERROR_SUCCESS)
			SYS_ERROR("RegMgr", "Failed to read at " + toStr(path) + "\\" + toStr(clave));
		return valor;
	}

	uint64_t RegMgr::Get_QWORD(std::wstring const& root,std::wstring const& path, std::wstring const& clave) {
		
		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) {
			SYS_ERROR("RegMgr", "Root '" + toStr(root) + "' is not recognized or supported.");
			return 0;
		}

		// Comprueba si está sobreescribiendo una clave con otro tipo. AVISA PERO CONTINÚA
		DWORD actualType = (DWORD)queryType(ptr_Root, path, clave);
		if (actualType != REG_NONE && actualType != REG_QWORD)
			SYS_WARN("RegMgr", "Type mismatch at " + toStr(path) + ": expected " + 
					regTypeName(REG_QWORD) + " but found " + regTypeName(actualType));

		// Obtiene el valor
		uint64_t valor = 0;
		DWORD size = sizeof(uint64_t);
		LSTATUS status = RegGetValueW(
			static_cast<HKEY>(ptr_Root),
			path.c_str(),
			clave.c_str(),
			RRF_RT_REG_QWORD, // Solo acepta QWORD
			nullptr,
			&valor,
			&size
		);
		if (status != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr", "Failed to read " + toStr(clave) + " from " + toStr(path) );
			return 0;
		}
		return valor;
	}

	std::wstring RegMgr::Get_SZ(std::wstring const& root,std::wstring const& path, std::wstring const& clave) {
		
		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) {
			SYS_ERROR("RegMgr", "Root '" + toStr(root) + "' is not recognized or supported.");
			return L"";
		}

		// Comprueba si está sobreescribiendo una clave con otro tipo. AVISA PERO CONTINÚA
		DWORD actualType = (DWORD)queryType(ptr_Root, path, clave);
		if (actualType != REG_NONE && actualType != REG_SZ)
			SYS_WARN("RegMgr", "Type mismatch at " + toStr(path) + ": expected " + 
					regTypeName(REG_SZ) + " but found " + regTypeName(actualType));

		// Primera llamada para obtener el tamaño necesario en bytes
		HKEY hRoot = static_cast<HKEY>(ptr_Root);
		DWORD size = 0;
		LSTATUS status = RegGetValueW(
			hRoot,
			path.c_str(),
			clave.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			nullptr,
			&size
		);
		if (status != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr","Failed to get size at " + toStr(path) + "\\" + toStr(clave));
			return L"";
		}
		// El tamaño debe ser al menos el de un terminador nulo (2 bytes)
    	if (size < sizeof(wchar_t)) {
			SYS_WARN("RegMgr","Readed 0 bytes at " + toStr(path) + "\\" + toStr(clave));
			return L"";
		}

		// El tamaño viene en bytes, lo pasamos a número de caracteres wchar_t
		std::wstring resultado;
		resultado.resize(size / sizeof(wchar_t) - 1); 

		// Segunda llamada para copiar el contenido
		status = RegGetValueW(
			hRoot,
			path.c_str(),
			clave.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			&resultado[0],
			&size
		);
		if (status != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr", "Failed to read at " + toStr(path) + "\\" + toStr(clave));
			return L"";
		}
		return resultado;
	}

	// Setters ------------------------------------------------------------------------------

	bool RegMgr::Set_DWORD(std::wstring const& root, std::wstring const& path, std::wstring const& clave, uint32_t valor) {

		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) return false;

		// Comprueba si está sobreescribiendo una clave con otro tipo. ERROR BLOQUEANTE
		DWORD actualType = (DWORD)queryType(ptr_Root, path, clave);
		if (actualType != REG_NONE && actualType != REG_DWORD) {
			SYS_ERROR("RegMgr", "Type mismatch: cannot write REG_DWORD over existing "
							+ std::string(regTypeName(actualType))
							+ " at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}

		// Establecer el valor
		if (RegSetKeyValueW(static_cast<HKEY>(ptr_Root), path.c_str(), clave.c_str(), REG_DWORD, &valor, sizeof(uint32_t)) != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr", "Failed to write REG_DWORD at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}
		return true;
	}

	bool RegMgr::Set_QWORD(std::wstring const& root, std::wstring const& path, std::wstring const& clave, uint64_t valor) {

		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) return false;

		// Comprueba si está sobreescribiendo una clave con otro tipo. ERROR BLOQUEANTE
		DWORD actualType = (DWORD)queryType(ptr_Root, path, clave);
		if (actualType != REG_NONE && actualType != REG_QWORD) {
			SYS_ERROR("RegMgr", "Type mismatch: cannot write REG_QWORD over existing "
							+ std::string(regTypeName(actualType))
							+ " at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}

		// Establecer el valor
		if (RegSetKeyValueW(static_cast<HKEY>(ptr_Root), path.c_str(), clave.c_str(), REG_QWORD, &valor, sizeof(uint64_t)) != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr", "Failed to write REG_QWORD at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}
		return true;
	}

	bool RegMgr::Set_STR(std::wstring const& root, std::wstring const& path, std::wstring const& clave, std::wstring const& valor) {
		// Comprueba valor de prefijo de ruta
		void* ptr_Root = ResolveRoot(root);
		if (!ptr_Root) return false;

		// Comprueba si está sobreescribiendo una clave con otro tipo. ERROR BLOQUEANTE
		DWORD actualType = (DWORD)queryType(hRoot, path, clave);
		if (actualType != REG_NONE && actualType != REG_SZ) {
			SYS_ERROR("RegMgr", "Type mismatch: cannot write REG_SZ over existing "
							+ std::string(regTypeName(actualType))
							+ " at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}

		// Establecer el valor
		DWORD sizeInBytes = static_cast<DWORD>((valor.size() + 1) * sizeof(wchar_t));
		if (RegSetKeyValueW(static_cast<HKEY>(ptr_Root), path.c_str(), clave.c_str(), REG_SZ, valor.c_str(), sizeInBytes) != ERROR_SUCCESS) {
			SYS_ERROR("RegMgr", "Failed to write REG_SZ at " + toStr(path) + "\\" + toStr(clave));
			return false;
		}
		return true;
	}

	// Otros --------------------------------------------------------------------------------

	void* RegMgr::ResolveRoot(std::wstring const& root) {
		// Se devuelve en void* y luego se castea a HKEY (debería ser lo mismo)
		if (root == L"HKEY_CURRENT_USER" 	|| root == L"HKCU")	return (void*)HKEY_CURRENT_USER;
		if (root == L"HKEY_LOCAL_MACHINE" 	|| root == L"HKLM")	return (void*)HKEY_LOCAL_MACHINE;
		if (root == L"HKEY_CLASSES_ROOT" 	|| root == L"HKCR")	return (void*)HKEY_CLASSES_ROOT;
		if (root == L"HKEY_USERS" 			|| root == L"HKU")	return (void*)HKEY_USERS;
		
		/*else*/
		SYS_ERROR("RegMgr","Invalid registry root: " + toStr(root));
		return nullptr;
	}

	uint32_t RegMgr::queryType(void* hRoot, const std::wstring& path, const std::wstring& clave) {
		DWORD type = REG_NONE;
		DWORD size = 0;
		RegGetValueW(hRoot, path.c_str(), clave.c_str(), RRF_RT_ANY, &type, nullptr, &size);
		return type;
	}

	const char* RegMgr::regTypeName(uint32_t type) {
		switch (type) {
			case (DWORD)REG_DWORD: return "REG_DWORD";
			case (DWORD)REG_QWORD: return "REG_QWORD";
			case (DWORD)REG_SZ:    return "REG_SZ";
			default:        return "REG_UNKNOWN";
		}
	}

	bool RegMgr::WaitUntilChange(std::wstring const& root, std::wstring const& path, std::wstring const& clave) {
		// Comprueba valor de prefijo de ruta
		HKEY hRoot = static_cast<HKEY>(ResolveRoot(root));
		if (!hRoot) return false;

		if (RegOpenKeyExW(hRoot, path.c_str(), 0, KEY_NOTIFY, &clave.c_str()) != ERROR_SUCCESS) return false;
		
		bool ok = (RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE) == ERROR_SUCCESS);
		RegCloseKey(hKey);
		return ok;
	}

#else
// ============================================================
//  Linux / macOS (Stubs)
// ============================================================

	RegMgr::RegMgr()  = default;
	RegMgr::~RegMgr() = default;

	uint32_t     RegMgr::Get_DWORD(std::wstring const&, std::wstring const&, std::wstring const&) { unsupported(); return 0;  }
	uint64_t     RegMgr::Get_QWORD(std::wstring const&, std::wstring const&, std::wstring const&) { unsupported(); return 0;  }
	std::wstring RegMgr::Get_SZ   (std::wstring const&, std::wstring const&, std::wstring const&) { unsupported(); return {}; }

	bool RegMgr::Set_DWORD(std::wstring const&, std::wstring const&, std::wstring const&, uint32_t)            { unsupported(); return false; }
	bool RegMgr::Set_QWORD(std::wstring const&, std::wstring const&, std::wstring const&, uint64_t)            { unsupported(); return false; }
	bool RegMgr::Set_STR  (std::wstring const&, std::wstring const&, std::wstring const&, std::wstring const&) { unsupported(); return false; }

	bool  		RegMgr::WaitUntilChange(std::wstring const&, std::wstring const&, std::wstring const&) 	{ unsupported(); return false;  }
	void* 		RegMgr::ResolveRoot(std::wstring const&)     											{ unsupported(); return nullptr; }
	uint32_t 	RegMgr::queryType(void*, const std::wstring&, const std::wstring&)						{ unsupported(); return 0; }
	const char* RegMgr::regTypeName(uint32_t)															{ unsupported(); return 0; }

#endif

std::string RegMgr::toStr(std::wstring const& ws) {
    return std::string(ws.begin(), ws.end());
}

inline void RegMgr::unsupported() {
	SYS_WARN("RegMgr", "Unsupported opperation.");
}