
# Toolchain para MSVC de Visual Studio

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Allow for using folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set runtime library to Multi-threaded DLL for MSVC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")

# Asegurar que CMake use UTF-8 por defecto
add_compile_options(/utf-8)
add_definitions(/D UNICODE /D _UNICODE)

# Agrupar archivos de cabecera, fuente y recursos en carpetas
source_group("Include"  FILES ${HEADERS})
source_group("Source"   FILES ${SOURCES})
source_group("Resources" FILES ${RESOURCES})

# Configuración específica para GLFW en MSVC
set(GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WIN32 ON CACHE BOOL "" FORCE)

