# Toolchain para builds nativos Linux (g++).
# Forzar uso de compilador C++ nativo (o cross si se instala en Windows).

if(UNIX AND NOT APPLE)
  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  find_program(SYSTEM_GXX NAMES g++ "/usr/bin/g++" ENV PATH)
  if(SYSTEM_GXX)
    set(CMAKE_CXX_COMPILER "${SYSTEM_GXX}" CACHE FILEPATH "Native Linux C++ compiler" FORCE)
  else()
    message(FATAL_ERROR "No se encontró g++ en el sistema. Instala g++ y vuelve a intentar.")
  endif()
else()
  # Si se usa desde Windows y hay un cross-compiler instalado (raro),
  # intentar localizar x86_64-linux-gnu-g++
  find_program(CROSS_GXX NAMES x86_64-linux-gnu-g++ ENV PATH)
  if(CROSS_GXX)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(CMAKE_CXX_COMPILER "${CROSS_GXX}" CACHE FILEPATH "Cross C++ compiler (Windows->Linux)" FORCE)
  else()
    message(FATAL_ERROR "linux-native toolchain sólo aplicable en Linux o si instalas un cross-compiler x86_64-linux-gnu-g++ en Windows (p. ej. via WSL/toolchains).")
  endif()
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "Default C++ standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)