#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <condition_variable>
#include <mutex>

/**
 * @class LogMgr
 * @brief A class for managing log files.
 *
 * The LogMgr class provides functionality for writing to log files.
 * It allows users to specify the log file name and the directory where the log file will be stored.
 */
class LogMgr
{
public:
	LogMgr(std::string const& filepath);
	~LogMgr();

	void enable(bool enable);
	void write(std::string txt, std::string whereIsFrom = "");
	void clear();

	std::string getFilePath() { return "#TODO"; }

private:

	std::string getTimestamp();

    /************ Variables ********************************************************/

	std::filesystem::path		filepath_;
	std::ofstream 	file_;			// Archivo
	bool 			enabled_;		// Flag para activar/desactivar la escritura
	std::string 	name_;
	bool			keep_open_;		// Mantiene el archivo abierto durante la ejecución

	std::mutex 					mtx_;
	std::condition_variable		cv_;
};
