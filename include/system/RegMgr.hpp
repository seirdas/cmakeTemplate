#pragma once
#include <string>
#include <windows.h>
#include <cstdint>

/**
 * @class RegMgr
 * @brief Clase para leer y escribir en el registro de Windows
 */
class RegMgr {

public:

// General ------------------------------------------------------------------------------

	/**
	 * @brief Constructor estándar
	 */
	RegMgr();

	/**
	 * @brief Destructor estándar
	 */
	~RegMgr();

// Getters ------------------------------------------------------------------------------

	/**
	 * @brief  Obtiene un valor del registro
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param dwvalor -- Valor DWORD 
 	 * @param qwvalor -- Valor QWORD
 	 * @param csvalor -- Valor string
	 * @return
	 */
	bool Get_REG(std::string path, std::string clave,  uint32_t * dwvalor, uint64_t  *qwvalor, std::string *csvalor);
	
	/**
	 * @brief  Obtiene un valor DWORD del registro 
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param valor -- Puntero a std::string donde se escribe el valor
	 * @return 
	 */
	bool Get_DWORD(std::string path, std::string clave, uint32_t * valor);

	/**
	 * @brief  Obtiene un valor std::string del registro
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param valor -- Puntero a std::string donde se escribe el valor
	 * @return 
	 */
	bool Get_STR(std::string path, std::string clave, std::string *valor);

// Setters ------------------------------------------------------------------------------

	/**
	 * @brief  Establece un valor del registro
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param dwvalor -- Valor DWORD 
 	 * @param qwvalor -- Valor QWORD 
 	 * @param csvalor -- Valor string 
	 * @return
	 */
	bool Set_REG(std::string path, std::string clave, uint32_t* dwvalor, uint64_t *qwvalor, std::string *csvalor);
	
	/**
	 * @brief  Establece un valor DWORD del registro 
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param valor -- Puntero a std::string donde se escribe el valor
	 * @return 
	 */
	bool Set_DWORD(std::string path, std::string clave, uint32_t valor);
	
	/**
	 * @brief  Establece un valor std::string del registro
	 * @param path -- Ruta del registro
	 * @param clave -- Clave de registro
	 * @param valor -- Puntero a std::string donde se escribe el valor
	 * @return 
	 */
	bool Set_STR(std::string path, std::string clave, std::string valor);
	
// Otros --------------------------------------------------------------------------------

	/**
 	* @brief Espera hasta que haya un cambio en la clave del registro indicada
 	* @param path -- Ruta del registro
	* @return 
	*/
	bool WaitUntilChange(std::string path);
};
