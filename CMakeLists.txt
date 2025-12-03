cmake_minimum_required(VERSION 3.10)

# Nombre del proyecto
project(MiProyect)

# Establecer el estándar de C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make Windows headers use wide character set
add_definitions(/D UNICODE /D _UNICODE)

# Encontrar todos los archivos .cpp, .h y .hpp en la carpeta src
file(GLOB_RECURSE SOURCES 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

# Crear un ejecutable a partir de los archivos .cpp
add_executable(${PROJECT_NAME} ${SOURCES})

# Incluir la carpeta src para los archivos de cabecera
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Colocar los binarios por configuración en executable/{configuration}/ dentro del build dir
# Ej.: executable/Debug/ejecutable.exe
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
)

# Configuraciones específicas para Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
	# Especifica el proyecto principal
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	# Allow for using folders
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()
