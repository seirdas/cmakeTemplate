# ======================================
# Toolchain para MSYS2 Clang-x64.
# ======================================

message(STATUS "Init Clang toolchain")

# Añadir clang al path para poder usar sus dependencias
set(ENV{PATH} "${CLANG_BIN};$ENV{PATH}")
set(CLANG_PATH "${CLANG_PATH}" CACHE STRING "Clang installation path" FORCE)
set(CLANG_BIN "${CLANG_BIN}" CACHE STRING "Clang binary path" FORCE)

set(CLANG_PATH "${CLANG_PATH}" CACHE STRING "Clang installation path" FORCE)
set(CLANG_BIN "${CLANG_PATH}/bin" CACHE STRING "Clang binary path" FORCE)

# Añadir clang al path para poder usar sus dependencias
set(ENV{PATH} "${CLANG_BIN};$ENV{PATH}")
set(CLANG_PATH "${CLANG_PATH}" CACHE STRING "Clang installation path" FORCE)
set(CLANG_BIN "${CLANG_BIN}" CACHE STRING "Clang binary path" FORCE)

set(CMAKE_CXX_FLAGS "-B${CLANG_BIN} ${CMAKE_CXX_FLAGS}" CACHE STRING "CXX Flags" FORCE)
set(CMAKE_C_FLAGS "-B${CLANG_BIN} ${CMAKE_C_FLAGS}" CACHE STRING "C Flags" FORCE)

# Establecer los compiladores, comprobando su existencia
#ESTE ES EL PARRAFO IMPORTANTE
set(CMAKE_C_COMPILER   "${CLANG_BIN}/clang.exe"      CACHE STRING "C Compiler" FORCE)
set(CMAKE_CXX_COMPILER "${CLANG_BIN}/clang++.exe"    CACHE STRING "CXX Compiler" FORCE)
set(CMAKE_RC_COMPILER  "${CLANG_BIN}/llvm-windres.exe" CACHE STRING "RC Compiler" FORCE)
set(CMAKE_MAKE_PROGRAM "${CLANG_BIN}/mingw32-make.exe" CACHE STRING "Make Program" FORCE)

# Mostrar mensajes
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
  if(EXISTS "${CMAKE_MAKE_PROGRAM}")
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
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Flags recomendados para MinGW
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")
set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")

# Definiciones comunes
add_definitions(-DUNICODE -D_UNICODE)

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