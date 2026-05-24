# ======================================
# Toolchain para MSYS2 Clang-x64.
# ======================================

message(STATUS "Init Clang toolchain")

# Compilación forzada a .exe de Windows, independientemente del SO donde se compile
if(NOT WIN32)
  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()

# Buscar las rutas de Clang y Clang/bin, no sobreescribir si ya lo obtuvo
if(NOT CLANG_PATH)
  set(CLANG_PATH "$ENV{CLANG_PATH}"   CACHE STRING "Clang installation path" FORCE)
endif()
set(CLANG_BIN  "${CLANG_PATH}/bin"  CACHE STRING "Clang binary path"       FORCE)

# Añadir clang al path para poder usar sus dependencias
set(ENV{PATH} "${CLANG_BIN};$ENV{PATH}")

# Opciones específicas de Clang (Ninja)
set(CMAKE_CXX_FLAGS "--target=x86_64-w64-windows-gnu ${CMAKE_CXX_FLAGS}"  CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS   "--target=x86_64-w64-windows-gnu ${CMAKE_C_FLAGS}"    CACHE STRING "" FORCE)

# Establecer los compiladores
set(CMAKE_C_COMPILER   "${CLANG_BIN}/clang.exe"         CACHE STRING "C Compiler"    FORCE)
set(CMAKE_CXX_COMPILER "${CLANG_BIN}/clang++.exe"       CACHE STRING "CXX Compiler"  FORCE)
set(CMAKE_RC_COMPILER  "${CLANG_BIN}/llvm-windres.exe"  CACHE STRING "RC Compiler"   FORCE)
if (CMAKE_GENERATOR MATCHES "Ninja")
    message(STATUS "Toolchain: Configuring with NINJA Generator")
    set(CMAKE_MAKE_PROGRAM "$ENV{NINJA_PATH}/ninja.exe" CACHE STRING "Ninja Make Program")
else()
    message(STATUS "Toolchain: Configuring with MAKE generator")
    set(CMAKE_MAKE_PROGRAM "${CLANG_BIN}/mingw32-make.exe" CACHE STRING "Make Program" FORCE)
endif()

# Mostrar mensajes de información
if(NOT CMAKE_TOOLCHAIN_FILE_PROCESSED)
  set(CMAKE_TOOLCHAIN_FILE_PROCESSED TRUE CACHE INTERNAL "Evitar doble mensaje")
  if ( EXISTS "${CMAKE_C_COMPILER}")
  message(STATUS "C Compiler found at ${CMAKE_C_COMPILER}.")
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
list(PREPEND CMAKE_PROGRAM_PATH "${CLANG_BIN}")

# Configurar búsqueda
set(CMAKE_FIND_ROOT_PATH "${CLANG_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Definiciones comunes
add_definitions(-DUNICODE -D_UNICODE)

