# Toolchain MinGW-w64 (UCRT variant)
# - En Linux: cross -> Windows con x86_64-w64-mingw32-g++
# - En Windows: busca en la ruta UCRT de MSYS2 (ucrt64) o en PATH.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(WIN32)
  find_program(MINGW_UCRT_GXX
    NAMES x86_64-w64-mingw32-g++
    PATHS "C:/msys64/ucrt64/bin"
    NO_DEFAULT_PATH
  )
  if(NOT MINGW_UCRT_GXX)
    find_program(MINGW_UCRT_GXX NAMES x86_64-w64-mingw32-g++ ENV PATH)
  endif()

  if(MINGW_UCRT_GXX)
    set(CMAKE_CXX_COMPILER "${MINGW_UCRT_GXX}" CACHE FILEPATH "MinGW-w64 UCRT C++ compiler" FORCE)
  else()
    message(WARNING "No se encontró x86_64-w64-mingw32-g++ en C:/msys64/ucrt64/bin ni en PATH. Ejecuta desde MSYS2 UCRT64 o añade la ruta al PATH.")
  endif()
else()
  # Linux host: usar cross-compiler instalado en el sistema (mingw-w64)
  set(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++" CACHE FILEPATH "MinGW-w64 UCRT cross C++ compiler" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "Default C++ standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)