# ======================================
# Toolchain para MSYS2 MinGW-x64.
# ======================================

message(STATUS "Init MinGW toolchain")

if(NOT WIN32)
  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()

set(MINGW_PATH "$ENV{MINGW_PATH}" CACHE STRING "MinGW installation path" FORCE)
set(MINGW_BIN "${MINGW_PATH}/bin" CACHE STRING "MinGW binary path" FORCE)

# Añadir mingw al path para poder usar sus dependencias
set(ENV{PATH} "${MINGW_BIN};$ENV{PATH}")
set(MINGW_PATH "${MINGW_PATH}" CACHE STRING "MinGW installation path" FORCE)
set(MINGW_BIN "${MINGW_BIN}" CACHE STRING "MinGW installation path" FORCE)

# Establecer los compiladores, comprobando su existencia
set(CMAKE_C_COMPILER    "${MINGW_BIN}/gcc.exe"      CACHE STRING "C Compiler"   FORCE)
set(CMAKE_CXX_COMPILER  "${MINGW_BIN}/g++.exe"      CACHE STRING "CXX Compiler" FORCE)
set(CMAKE_RC_COMPILER   "${MINGW_BIN}/windres.exe"  CACHE STRING "RC Compiler"  FORCE)
if (CMAKE_GENERATOR MATCHES "Ninja")
    message(STATUS "Toolchain: Configuring with NINJA Generator")
    set(CMAKE_MAKE_PROGRAM "$ENV{NINJA_PATH}/ninja.exe" CACHE STRING "Ninja Make Program")
else()
    message(STATUS "Toolchain: Configuring with MAKE generator")
    set(CMAKE_MAKE_PROGRAM "${MINGW_BIN}/mingw32-make.exe" CACHE STRING "Make Program" FORCE)
endif()

# Mostrar mensajes
if(NOT CMAKE_TOOLCHAIN_FILE_PROCESSED)
  set(CMAKE_TOOLCHAIN_FILE_PROCESSED TRUE CACHE INTERNAL "Evitar doble mensaje")
  if (EXISTS "${CMAKE_C_COMPILER}")
  message(STATUS "C Compiler found at ${CMAKE_C_COMPILER}")
  endif()
  if (EXISTS "${CMAKE_CXX_COMPILER}")
  message(STATUS "CXX Compiler found at ${CMAKE_CXX_COMPILER}")
  endif()
  if (EXISTS "${CMAKE_RC_COMPILER}")
  message(STATUS "RC Compiler found at ${CMAKE_RC_COMPILER}")
  endif()
  if (EXISTS "${CMAKE_MAKE_PROGRAM}")
  message(STATUS "Make Program found at ${CMAKE_MAKE_PROGRAM}")
  endif()
endif()

# Especificar que windres usa formato GNU para flags
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -O coff -i <SOURCE> -o <OBJECT>"
)

# Por defecto anteponer la ruta 
# para que g++ y sus DLLs se resuelvan durante configure / try_compile.
if(NOT DEFINED MINGW_MINGW64_PREPEND)
  set(MINGW_MINGW64_PREPEND ON)
endif()
list(PREPEND CMAKE_PROGRAM_PATH "${MINGW_BIN}")

# Configurar búsqueda
set(CMAKE_FIND_ROOT_PATH "${MINGW_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Definiciones comunes
add_definitions(-DUNICODE -D_UNICODE)

# Descomentar esto para pararse y debugear este archivo.
# message(FATAL_ERROR debugStop)
