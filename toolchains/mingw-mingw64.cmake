# Toolchain para MinGW-w64 (mingw64) � uso Windows -> Windows con MSYS2 MinGW64.
# Localiza x86_64-w64-mingw32-g++ instalado por MSYS2 y lo configura como C++ compiler.
#
# Uso recomendado:
# - Ejecuta CMake desde la shell "MSYS2 MinGW 64-bit" (ruta puesta en PATH).
# - O bien pasa la ruta expl�cita:
#     -D MINGW_MINGW64_BIN="C:/msys64/mingw64/bin"
#   o exporta variable de entorno MINGW_MINGW64_BIN antes de invocar CMake.
#
# Opcional: MINGW_MINGW64_PREPEND para anteponer una ruta temporal al PATH
# (afecta s�lo al proceso de configuraci�n).

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_PATH "C:/msys64/mingw64")
set(MINGW_BIN "${MINGW_PATH}/bin")

# Verificar que exista el directorio
if(NOT EXISTS "${MINGW_BIN}")
    message(FATAL_ERROR "MinGW no encontrado en: ${MINGW_BIN}")
endif()

# Establecer compiladores
set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe")
set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe")
set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe")
# Especificar que windres usa formato GNU para flags
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -O coff -i <SOURCE> -o <OBJECT>"
)
# Ninja puede estar en otro lugar, buscar alternativas
find_program(NINJA_EXE ninja PATHS "${MINGW_BIN}" "C:/Program Files/Ninja" "C:/ninja")
if(NINJA_EXE)
    message(STATUS "Ninja encontrado en: ${NINJA_EXE}")
    set(CMAKE_MAKE_PROGRAM "${NINJA_EXE}")
else()
    # Si no hay ninja, usar mingw32-make
    set(CMAKE_MAKE_PROGRAM "${MINGW_BIN}/mingw32-make.exe")
    message(STATUS "Ninja no encontrado, usando mingw32-make")
endif()



# Por defecto anteponer la ruta para que g++ y sus DLLs se resuelvan durante configure / try_compile.
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
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++")