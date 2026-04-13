#include "system/LogMgr.hpp"

#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;


// General ______________________________________________________________________________
LogMgr::LogMgr(std::string const& filepath) :
	filepath_(filepath), 
	enabled_(true), 
	keep_open_(true)
{
	if (filepath_.has_parent_path()) {
        fs::create_directories(filepath_.parent_path());
    }
	write("--- Log Init ---");
}

LogMgr::~LogMgr() {
	// Cerrar archivo si estába abierto
	if (file_.is_open()) file_.close();
}


void LogMgr::enable(bool sel) {
	enabled_ = sel;

	if (!file_.is_open()) 
		file_.open(filepath_, std::ios_base::out | std::ios_base::app);
	
	if (file_.is_open()) {
		write("Log enabled");
	}

	if (!keep_open_ && file_.is_open()) 
		file_.close();
}

void LogMgr::write(std::string txt)
{
	if (!enabled_) return;

	// Bloqueo para evitar problemas con hilos (Thread-safe)
    std::lock_guard<std::mutex> lock(mtx_);

	if (file_.is_open()) 
		file_.close();

	file_.open(filepath_, std::ios_base::out | std::ios_base::app);

	std::string out = getTimestamp();
	out += " " + txt;

	if (file_.is_open()) {
		file_ << out << std::endl;
		file_.flush(); // Asegura que se escriba en el disco de inmediato
	}

	if(!keep_open_) file_.close();
}


void LogMgr::clear() {

    std::lock_guard<std::mutex> lock(mtx_);
	file_.open(filepath_, std::ios_base::out | std::ios_base::trunc);

	if (file_.is_open()) {
		write("");
		file_.close();
	}
}

std::string LogMgr::getTimestamp()
{
	// Obtener fecha y hora del sistema
    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    // Convertir a estructura local (formato legible)
    std::tm bt = *std::localtime(&now_time);
    
    // Darle formato: [AAAA-MM-DD HH:MM:SS]
    std::ostringstream oss;
    oss << std::put_time(&bt, "[%Y-%m-%d %H:%M:%S]");
    return oss.str();
}
