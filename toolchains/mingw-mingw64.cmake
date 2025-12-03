# Toolchain para MinGW-w64 (mingw64) — uso Windows -> Windows con MSYS2 MinGW64.
# Localiza x86_64-w64-mingw32-g++ instalado por MSYS2 y lo configura como C++ compiler.
#
# Uso recomendado:
# - Ejecuta CMake desde la shell "MSYS2 MinGW 64-bit" (ruta puesta en PATH).
# - O bien pasa la ruta explícita:
#     -D MINGW_MINGW64_BIN="C:/msys64/mingw64/bin"
#   o exporta variable de entorno MINGW_MINGW64_BIN antes de invocar CMake.
#
# Opcional: MINGW_MINGW64_PREPEND para anteponer una ruta temporal al PATH
# (afecta sólo al proceso de configuración).

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Detectar MINGW_MINGW64_BIN si no está definido
if(NOT DEFINED MINGW_MINGW64_BIN)
  if(DEFINED ENV{MINGW_MINGW64_BIN})
    set(MINGW_MINGW64_BIN $ENV{MINGW_MINGW64_BIN})
  elseif(EXISTS "C:/msys64/mingw64/bin")
    set(MINGW_MINGW64_BIN "C:/msys64/mingw64/bin")
  endif()
endif()

# Por defecto anteponer la ruta para que g++ y sus DLLs se resuelvan durante configure / try_compile.
if(NOT DEFINED MINGW_MINGW64_PREPEND)
  set(MINGW_MINGW64_PREPEND ON)
endif()

if(DEFINED MINGW_MINGW64_BIN)
  # Asegurar rutas absolutas al compilador
  set(CMAKE_C_COMPILER "${MINGW_MINGW64_BIN}/x86_64-w64-mingw32-gcc.exe" CACHE FILEPATH "C compiler" FORCE)
  set(CMAKE_CXX_COMPILER "${MINGW_MINGW64_BIN}/x86_64-w64-mingw32-g++.exe" CACHE FILEPATH "CXX compiler" FORCE)

  if(MINGW_MINGW64_PREPEND)
    # Anteponer la ruta al PATH del proceso de CMake para que las DLLs estén disponibles.
    set(ENV{PATH} "${MINGW_MINGW64_BIN};$ENV{PATH}")
  endif()
endif()

# Estándar C++
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Default C++ standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Para builds nativos en Windows con MinGW no fuerzo modos de búsqueda especiales.
# (Si necesitas comportamiento de cross-build, usa un toolchain distinto).