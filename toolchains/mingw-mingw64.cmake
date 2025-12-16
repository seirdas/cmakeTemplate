# Toolchain para MSYS2 MinGW-x64.
#

# Function to read the configuration file and set variables
function(readConfigFile FILE)
    # Read and parse the .ini file
    file(STRINGS "${FILE}" ini_contents)

    # Process each line
    foreach(line ${ini_contents})
        # Skip comments and empty lines
        if(line MATCHES "^;" OR line STREQUAL "")
            continue()
        endif()
        # Handle sections (though we won't use them directly)
        if(line MATCHES "^\\[.*\\]$")
            continue()
        endif()

        # Remove whitespace
        string(STRIP ${line} line)

        # Split key-value pairs
        if(line MATCHES "^(.*)=(.*)$")
            string(REGEX REPLACE "^(.*)=(.*)$" "\\1" key ${line})
            string(REGEX REPLACE "^(.*)=(.*)$" "\\2" value ${line})
            string(STRIP ${key} key)
            string(STRIP ${value} value)

            # Set the variable in CMake
            set(${key} "${value}" CACHE STRING "Set from config file" FORCE)
            # message(STATUS "Set variable ${key} to ${value}")
        endif()
    endforeach()
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.ini")
    readConfigFile("${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.ini")   # Read the configuration file
endif()
    
# No necesario: Si no se lee bien la ruta del .ini, se pone la ruta del mingw de msys por defecto
if(NOT EXISTS ${MINGW_PATH})
    set(MINGW_PATH "C:/msys64/mingw64")
endif()

# Añadir el Path de mingw para poder usar sus dependencias
set(MINGW_BIN "${MINGW_PATH}/bin")
set(ENV{PATH} "${MINGW_BIN};$ENV{PATH}")

set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe" CACHE STRING "C Compiler" FORCE)
set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe" CACHE STRING "CXX Compiler" FORCE)
set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe" CACHE STRING "RC Compiler" FORCE)
set(CMAKE_MAKE_PROGRAM "${MINGW_BIN}/mingw32-make.exe" CACHE STRING "Make Program" FORCE)

set(CMAKE_CXX_FLAGS "-B${MINGW_BIN} ${CMAKE_CXX_FLAGS}" CACHE STRING "CXX Flags" FORCE)
set(CMAKE_C_FLAGS "-B${MINGW_BIN} ${CMAKE_C_FLAGS}" CACHE STRING "C Flags" FORCE)

message(STATUS CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER}")
message(STATUS CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER}")
message(STATUS CMAKE_RC_COMPILER: " ${CMAKE_RC_COMPILER}")
message(STATUS CMAKE_MAKE_PROGRAM: " ${CMAKE_MAKE_PROGRAM}")

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Especificar que windres usa formato GNU para flags
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -O coff -i <SOURCE> -o <OBJECT>"
)

# Por defecto anteponer la ruta 
# para que g++ y sus DLLs se resuelvan durante configure / try_compile.
if(NOT DEFINED MINGW_MINGW64_PREPEND)
  set(MINGW_MINGW64_PREPEND ON)
endif()


# Configurar búsqueda
set(CMAKE_FIND_ROOT_PATH "${MINGW_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Flags recomendados para MinGW
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")
set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")



# Ninja puede estar en otro lugar, buscar alternativas
# find_program(NINJA_EXE ninja PATHS "${MINGW_BIN}" "C:/Program Files/Ninja" "C:/ninja")
# if(NINJA_EXE)
#     message(STATUS "Ninja encontrado en: ${NINJA_EXE}")
#     set(CMAKE_MAKE_PROGRAM "${NINJA_EXE}")
# else()
#     # Si no hay ninja, usar mingw32-make
#     set(CMAKE_MAKE_PROGRAM "${MINGW_BIN}/mingw32-make.exe")
#     message(STATUS "Ninja no encontrado, usando mingw32-make")
# endif()