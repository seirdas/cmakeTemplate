# Toolchain para MSYS2 MinGW-x64.
#

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)


# Especificar que windres usa formato GNU para flags
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -O coff -i <SOURCE> -o <OBJECT>"
)


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
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++")