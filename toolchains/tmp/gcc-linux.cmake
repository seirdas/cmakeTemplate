# Toolchain adaptable: sólo C++.
# Por defecto compila para Linux (nativo). Para Windows exporta TARGET=Windows
if(DEFINED ENV{TARGET} AND "$ENV{TARGET}" STREQUAL "Windows")
  # Cross-compile to Windows using mingw-w64 (instala x86_64-w64-mingw32-* en Linux)
  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)

  # Sólo C++ compiler
  set(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++" CACHE FILEPATH "Cross C++ compiler" FORCE)

  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
else()
  # Native Linux
  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)

  # Sólo C++ compiler
  set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE FILEPATH "Native C++ compiler" FORCE)

  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# Opciones C++ por defecto (puedes ajustar según tu proyecto)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Default C++ standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)