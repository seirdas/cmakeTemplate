#pragma once

#include <string>

// Definición de códigos de escape ANSI para colores
const std::string ANSI_RESET      		= "\033[0m";
const std::string ANSI_BLACK      		= "\033[30m";
const std::string ANSI_RED        		= "\033[31m";
const std::string ANSI_GREEN      		= "\033[32m";
const std::string ANSI_YELLOW     		= "\033[33m";
const std::string ANSI_BLUE       		= "\033[34m";
const std::string ANSI_MAGENTA    		= "\033[35m";
const std::string ANSI_CYAN       		= "\033[36m";
const std::string ANSI_WHITE      		= "\033[37m";
const std::string ANSI_BRIGHT_RED     	= "\033[91m";
const std::string ANSI_BRIGHT_GREEN   	= "\033[92m";
const std::string ANSI_BRIGHT_YELLOW  	= "\033[93m";
const std::string ANSI_BRIGHT_BLUE    	= "\033[94m";
const std::string ANSI_BRIGHT_MAGENTA 	= "\033[95m";
const std::string ANSI_BRIGHT_CYAN    	= "\033[96m";
const std::string ANSI_BOLD           	= "\033[1m";
const std::string ANSI_UNDERLINE      	= "\033[4m";

// Secuencias de control de línea y cursor
const std::string ANSI_CLEAR_TO_EOL     = "\033[K";