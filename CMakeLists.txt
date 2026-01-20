
# ==============================
# CMakeLists.txt - Configuración del proyecto CMake
# ==============================

# CMake minimum version and project setup
cmake_minimum_required(VERSION 3.10)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Establecer estándar C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Establecer nombre del exe igual que el nombre de la carpeta del proyecto
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME CACHE)
project(${PROJECT_NAME})


if (MSVC)
  # Definiciones de compatibilidad
  add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS # Desactivar advertencias de funciones inseguras
      NOMINMAX                # Evitar definición de macros min y max
      WIN32_LEAN_AND_MEAN     # Excluir APIs innecesarias de Windows.h
  )

  message(STATUS "Using MSVC dynamic runtime (/MD, /MDd)")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

endif()



# ===============================
# Configuración del proyecto (código fuente propio)
# ==============================

# Encontrar todos los archivos .cpp, .h y .hpp
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")
set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/external)
file(MAKE_DIRECTORY ${EXTERNAL_PATH})

message(STATUS ${CMAKE_SOURCE_DIR})
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})


# ===============================
# Librerías externas
# ===============================
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)                              # menos salida en consola
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")  # Evitar que FetchContent vuelva a actualizar el repositorio cada vez

# Forzamos a FetchContent a usar ruta común 
## set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/external")
message(STATUS "FETCHCONTENT_BASE_DIR en ${FETCHCONTENT_BASE_DIR}")

# ------------------------------
# LIBRERÍA GRÁFICA CON GLFW + IMGUI + OTRAS DEPENDENCIAS
# ------------------------------

set(GLOBAL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external")

# GLFW (Ventanas) ___________________________
message(STATUS "Fetching GLFW library...")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # No construir ejemplos
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)    # No construir tests
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)    # No construir documentación
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)    # No instalar
if (MSVC)
  set(GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW    TRUE         # habilita --depth 1
  SOURCE_DIR     "${GLOBAL_SOURCE_DIR}/glfw_src"
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(glfw)
find_package(OpenGL REQUIRED)   # Incluir librería de OpenGL (GLFW la usa para renderizar)


# IMGUI (Interfaz) ___________________________
message(STATUS "Fetching ImGui library...")
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.5-docking      # o la versión que necesites
  GIT_SHALLOW    TRUE          # habilita --depth 1
  SOURCE_DIR     "${GLOBAL_SOURCE_DIR}/imgui_src"
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_DIR)
file(GLOB IMGUI_SOURCES 
  "${IMGUI_DIR}/*.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp" 
)

# Crear librería estática con ImGui + GLFW ____________________________
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_link_libraries(imgui_lib PUBLIC 
  glfw                # GLFW
  OpenGL::GL          # OpenGL                
  $<$<PLATFORM_ID:Windows>:user32 gdi32 shell32>  # Librerías de sistema en Windows
)
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)   # Directorios propios
# target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)      # Usar GLAD como cargador de OpenGL

# -------------------------------
# Librería de asio (redes) con dependencias
# -------------------------------

# Asio (Red - Standalone)
FetchContent_Declare(
    asio_network
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-36-0
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${GLOBAL_SOURCE_DIR}/asio_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(asio_network)
# Crear una librería estática de red + Vincular con Winsock2 en Windows
add_library(asio_lib INTERFACE)
target_include_directories(asio_lib INTERFACE "${asio_network_SOURCE_DIR}/asio/include")  # Incluir headers de Asio
target_compile_definitions(asio_lib INTERFACE ASIO_STANDALONE)                            # Definir ASIO_STANDALONE para usar Asio sin Boost
target_link_libraries(asio_lib INTERFACE
  $<$<PLATFORM_ID:Windows>:ws2_32 mswsock>  # Librerías de sockets en Windows
)     

# -------------------------------
# Librería de nlohmann/json (JSON) 
# -------------------------------

# Descarga el archivo json.hpp del último commit de github (si no existe)
# Se podría hacer también con el FetchContent pero tarda más
file(MAKE_DIRECTORY ${EXTERNAL_PATH}/nlohmann)
if (NOT EXISTS ${EXTERNAL_PATH}/nlohmann/json.hpp)
  message(STATUS "Downloading lastest release of nlohmann/json.hpp...")
  file(DOWNLOAD 
    "https://github.com/nlohmann/json/releases/latest/download/json.hpp"
    "${EXTERNAL_PATH}/nlohmann/json.hpp"
    STATUS DOWNLOAD_STATUS
  )
  if(NOT DOWNLOAD_STATUS EQUAL 0)
      message(FATAL_ERROR "Error downloading json.hpp")
  endif()
else()
  message(STATUS "nlohmann's json.hpp found")
endif()
# Crear una librería de interfaz para nlohmann/json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann
)


# -------------------------------
# Librería de Miniaudio (Audio)
# -------------------------------

message(STATUS "Fetching Miniaudio library...")
set(MA_ENABLE_VORBIS  OFF CACHE BOOL "" FORCE)  # No construir soporte para Vorbis
set(MA_ENABLE_OPUS    OFF CACHE BOOL "" FORCE)  # No construir soporte para Opus
set(MA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)  # No construir ejemplos
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.23          # o la versión que necesites
  GIT_SHALLOW    TRUE            # habilita --depth 1
  SOURCE_DIR     "${GLOBAL_SOURCE_DIR}/miniaudio_src"
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(miniaudio)
# Crear una librería estática con Miniaudio
add_library(miniaudio_lib STATIC
  ${miniaudio_SOURCE_DIR}/miniaudio.c
)
target_include_directories(miniaudio_lib PUBLIC ${miniaudio_SOURCE_DIR})
target_link_libraries(miniaudio_lib PUBLIC
  $<$<PLATFORM_ID:Windows>:winmm>  
  $<$<PLATFORM_ID:Linux>:pthread dl m>
)

# -------------------------------
# Librería DaisySP (DSP)
# -------------------------------

message(STATUS "Fetching DaisySP library...")
set(BUILD_DAISYSP_EXAMPLES  OFF CACHE BOOL "" FORCE)    # No construir ejemplos
FetchContent_Declare(
  daisysp
  GIT_REPOSITORY https://github.com/electro-smith/DaisySP.git
  GIT_TAG        V1.0.0
  GIT_SHALLOW    TRUE
  SOURCE_DIR     "${GLOBAL_SOURCE_DIR}/daisysp_src"
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(daisysp)

file(GLOB_RECURSE DAISYSP_SOURCES
  "${daisysp_SOURCE_DIR}/DaisySP-LGPL/Source/*.cpp"
)
# Crear una librería estática con DaisySP
add_library(daisysp_lib STATIC ${DAISYSP_SOURCES})
target_compile_definitions(daisysp_lib PUBLIC
  DAISYSP_DESKTOP=1   # Supone entorno de escritorio, no sistema embebido
  _USE_MATH_DEFINES
)
target_include_directories(daisysp_lib PUBLIC
  "${daisysp_SOURCE_DIR}/Include"
)
if (MSVC)
  target_compile_options(daisysp_lib PRIVATE /fp:fast)
else()
  target_compile_options(daisysp_lib PRIVATE -ffast-math)
endif()
target_link_libraries(daisysp_lib PRIVATE
    $<$<PLATFORM_ID:Linux>:asound>
)





# ===============================
# Crear el ejecutable
# ===============================

# Crear el ejecutable principal
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
# Rutas de archivos de cabecera del proyecto (archivos .h propios)
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/resources
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external
)
# Enlazar librerías externas
target_link_libraries(${PROJECT_NAME} PRIVATE 
  imgui_lib     # Librería creada con ImGui + GLFW + OpenGL
  asio_lib      # Librería creada con Asio + Winsock2 (Windows)
  nlohmann_json # Librería creada con nlohmann/json
  miniaudio_lib # Librería creada con Miniaudio + winmm (Windows)
  daisysp_lib   # Librería creada con DaisySP ()
)



# ===============================
# Configuración específica del compilador/SO
# ===============================

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    # user32    # Ventanas y controles básicos de Windows (Ya en imgui_lib)
    # gdi32     # Gráficos básicos de Windows (Ya en imgui_lib)
    # shell32   # Funciones de shell de Windows (Ya en imgui_lib)
    imm32       # Soporte de IME (Input Method Editor) para entrada de texto avanzada
    ole32       # Component Object Model (COM) y OLE
    oleaut32    # Automatización OLE
    uuid        # Soporte para GUIDs y UUIDs
    version     # Información de versión de archivos ejecutables
    dwmapi      # Integración con el gestor de ventanas DWM
   )
endif()

# Configuración específica para MSVC
if(MSVC)
  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
    /W3                       # Nivel de advertencia 3
    /EHsc                     # Manejo de excepciones en C++
    $<$<CONFIG:Debug>:/ZI>    # Información de depuración
    $<$<CONFIG:Debug>:/Od>    # Desactivar optimizaciones en Debug
    $<$<CONFIG:Release>:/O2>  # Optimización para velocidad en Release
  )

  # Especifica el proyecto principal
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  # 
  set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
  # Usar el main normal aunque sea una App de Windows
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
  # Agrupar archivos de cabecera, fuente y recursos en carpetas
  source_group("Include"    FILES ${HEADERS})
  source_group("Source"     FILES ${SOURCES})
  source_group("Resources"  FILES ${RESOURCES})


# Configuración específica para GCC/MinGW
elseif(MINGW)
  # Librerías de sistema 
  target_link_libraries(${PROJECT_NAME} PRIVATE
    winpthread  # Hilos en Windows
    kernel32    # Funciones básicas de Windows
    comdlg32    # Diálogos comunes de Windows
  )
endif()


# ===============================
# Configuración post-build
# ===============================

# Copia todo el contenido de la carpeta 'config' a la carpeta del binario
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
# Configurar carpetas de salida del ejecutable y librerías
set_target_properties(${PROJECT_NAME} PROPERTIES 
  WIN32_EXECUTABLE $<BOOL:${WIN32}>     # Indica si es una aplicación de Windows (sin consola)
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida del ejecutable
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías estáticas
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías dinámicas
)
