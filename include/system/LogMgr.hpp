#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <mutex>

/**
 * @class LogMgr
 * @brief Clase para escribir en archivos de log
 */
class LogMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor de la clase LogMgr.
     * @param filepath Ruta completa o relativa del archivo de log.
     * Si las carpetas del path no existen, el constructor intentará crearlas.
     */
    LogMgr(std::string const& filepath);

    /**
     * @brief Destructor de la clase.
     * Se asegura de cerrar correctamente el flujo del archivo si estaba abierto.
     */
    ~LogMgr();


// Log ----------------------------------------------------------------------------------

    /**
     * @brief Activa o desactiva la escritura en el archivo de log.
     * @param enable True para habilitar, False para silenciar el log.
     */
    void enable(bool enable);

    /**
     * @brief Escribe un mensaje en el archivo de log.
     * El mensaje se guardará automáticamente con un timestamp al inicio.
     * Esta función es thread-safe mediante el uso de mutex.
     * * @param txt Texto que se desea registrar.
     */
    void write(std::string const& txt);

    /**
     * @brief Borra el contenido actual del archivo de log.
     * Abre el archivo en modo truncado y deja una línea vacía inicial.
     */
    void clear();

    
// Propiedades de archivo ---------------------------------------------------------------

    /**
     * @brief Obtiene la ruta del archivo de log actual.
     * @return std::string con la ruta del archivo.
     */
    std::string getFilePath() const;

    /**
     * @brief Devuelve el nombre del archivo
     * @return Nombre del archivo
     */
    std::string getName() const;

private:

// Utilidades ---------------------------------------------------------------------------

    /**
     * @brief Genera una cadena de texto con la fecha y hora actual.
     * @return Formato: [AAAA-MM-DD HH:MM:SS]
     */
    std::string getTimestamp();


/************ Variables ****************************************************************/

	std::ofstream 			file_;			// Archivo (literalmente)
	std::filesystem::path	filepath_;		// Ruta del archivo
	std::string 			name_;			// Nombre del archivo
	bool 					enabled_;		// Flag para activar/desactivar la escritura
	bool					keep_open_;		// Mantiene el archivo abierto durante la ejecución
	std::mutex 				mtx_;			// Mutex de operaciones sobre archivo
};
