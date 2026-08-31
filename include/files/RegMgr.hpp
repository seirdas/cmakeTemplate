#pragma once

#include <string>
#include <cstdint>		// provides uint32_t, uint64_t


// Macros de conveniencia para leer/escribir en el registro
#define REG_GET_DWORD(root, path, clave)  		RegMgr::instance().Get_DWORD(root, path, clave)
#define REG_GET_QWORD(root, path, clave)  		RegMgr::instance().Get_QWORD(root, path, clave)
#define REG_GET_SZ(root, path, clave)     		RegMgr::instance().Get_SZ(root, path, clave)
#define REG_SET_DWORD(root, path, clave, val) 	RegMgr::instance().Set_DWORD(root, path, clave, val)
#define REG_SET_QWORD(root, path, clave, val) 	RegMgr::instance().Set_QWORD(root, path, clave, val)
#define REG_SET_STR(root, path, clave, val)   	RegMgr::instance().Set_STR(root, path, clave, val)

// Macros de conveniencia para ruta raíz (root) de parámetro de funciones
#define HKCU	L"HKEY_CURRENT_USER"
#define HKLM 	L"HKEY_LOCAL_MACHINE"
#define HKCR 	L"HKEY_CLASSES_ROOT"
#define HKU 	L"HKEY_USERS"


/**
 * @class RegMgr
 * @brief Clase singleton para leer y escribir en el registro de Windows
 * @note  En Linux/macOS todos los métodos son stubs que emiten SYS_WARN.
 */
class RegMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Declara la instancia estática de la clase
     */
    static RegMgr& instance();


// Getters ------------------------------------------------------------------------------

	/**
	 * @brief  Obtiene un valor DWORD del registro 
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @return Valor uint32_t == REG_DWORD
	 */
	uint32_t Get_DWORD(std::wstring const& root,std::wstring const& path, std::wstring const& clave);

	/**
	 * @brief  Obtiene un valor QWORD del registro
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @return Valor uint64_t == REG_QWORD
	 */
	uint64_t Get_QWORD(std::wstring const& root,std::wstring const& path, std::wstring const& clave);

	/**
	 * @brief  Obtiene un valor std::string del registro
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @return Valor std::wstring == REG_SZ
	 */
	std::wstring Get_SZ(std::wstring const& root,std::wstring const& path, std::wstring const& clave);


// Setters ------------------------------------------------------------------------------

	/**
	 * @brief  Establece un valor DWORD en el registro
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @param valor Valor uint32_t == DWORD a escribir
	 * @return true si se ha ejecutado correctamente, false en caso contrario
	 */
	bool Set_DWORD(std::wstring const& root, std::wstring const& path, std::wstring const& clave, uint32_t valor);

	/**
	 * @brief  Establece un valor DWORD en el registro 
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @param valor Valor uint32_t == DWORD a escribir
	 * @return true si se ha ejecutado correctamente, false en caso contrario
	 */
	bool Set_QWORD(std::wstring const& root, std::wstring const& path, std::wstring const& clave, uint64_t valor);
	
	/**
	 * @brief  Establece un valor SZ (string) en el registro 
	 * @param root Prefijo de ruta. Admite: 
	 * 	HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT, HKEY_USERS
	 * 	HKCU, HKLM, HKCR, HKU (equivalentes respectivamente)
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 * @param valor Valor std::wstring == REG_SZ a escribir
	 * @return true si se ha ejecutado correctamente, false en caso contrario
	 */
	bool Set_STR(std::wstring const& root, std::wstring const& path, std::wstring const& clave, std::wstring const& valor);
	

// Otros --------------------------------------------------------------------------------

	/**
 	* @brief Espera hasta que haya un cambio en la clave del registro indicada
 	* @param path Ruta del registro
	* @return true si la clave de registro ha cambiado
	*/
	bool WaitUntilChange(std::wstring const& root, std::wstring const& path);

	
private:

// General ------------------------------------------------------------------------------

	/**
	 * @brief Constructor estándar
	 */
	RegMgr();

	/**
	 * @brief Destructor estándar
	 */
	~RegMgr();

	/**
     * @brief Evitar copias
     */
	RegMgr(const RegMgr&)            = delete;
    RegMgr& operator=(const RegMgr&) = delete;


// Otros (privado) ----------------------------------------------------------------------

	/**
	 * @brief Obtiene el prefijo de ruta de registro a partir de un wstring
	 * @param root Prefijo de ruta de registro en formato wstring
	 * @return void*, equivalente a HKEY
	 */
	void* resolve_root(std::wstring const& path);

	/**
	 * @brief Consulta el tipo de una clave sin leer su valor.
	 * @returns REG_NONE si no existe o hay error.
	 * @param root Prefijo de ruta en formato 
	 * @param path Ruta del registro
	 * @param clave Clave de registro
	 */
	unsigned int query_type(void* hRoot, const std::wstring& path, const std::wstring& clave);

	/**
	 * @brief Nombre legible del tipo de registro para mensajes de error
	  */
	const char* reg_typename(unsigned int type);


// Comunes por SO -----------------------------------------------------------------------

	/**
	 * @brief Convierte rutas de registro ASCII wstring a string  
	 */
	std::string toStr(std::wstring const& ws);

	/**
	 * @brief Genera mensaje de "no soportado" para SO != Windows 
	 */
	inline void unsupported();
};

