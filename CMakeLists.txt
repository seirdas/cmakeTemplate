cmake_minimum_required(VERSION 3.10)

# Nombre del proyecto
# Get the name of the root folder
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
# Set the project name
project(${PROJECT_NAME})


######  FUNCTION DEFINITIONS ######
# Function to clear all user-defined variables except CMake built-in variables
function(clearVars)
    # Clear all user-defined variables
    get_cmake_property(ALL_VARIABLES VARIABLES)
    foreach(VAR ${ALL_VARIABLES})
        if(NOT VAR MATCHES "^CMAKE_")  # Skip CMake built-in variables
            unset(${VAR} CACHE)  # Unset the variable from the cache
        endif()
    endforeach()
	if(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt")
        file(REMOVE "${CMAKE_BINARY_DIR}/CMakeCache.txt")
    endif()
endfunction()
######

clearVars()

add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/build"
    COMMENT "Cleaning all build directories"
)

# Establecer el estándar de C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make Windows headers use wide character set
add_definitions(/D UNICODE /D _UNICODE)

# Encontrar todos los archivos .cpp, .h y .hpp en la carpeta src
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

# Crear un ejecutable a partir de los archivos .cpp
add_executable(${PROJECT_NAME} ${SOURCES})

# Incluir la carpeta include para los archivos de cabecera
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
	# Set runtime library to Multi-threaded DLL for MSVC
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
