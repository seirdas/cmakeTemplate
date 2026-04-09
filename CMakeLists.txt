
# ==============================
# CMakeLists.txt - Configuración del proyecto CMake
# ==============================

# CMake minimum version and project setup
cmake_minimum_required(VERSION 3.10)

# Activa configuración dependiente de dlls (no static) para compilación MSVC (Visual Studio)
if (MSVC)
  message(STATUS "Force using latest SDK for Visual Studio")
  set(CMAKE_SYSTEM_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} CACHE STRING "Windows SDK version" FORCE)
  set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0")

  # Flag para mejorar la compatibilidad de headers antiguos
  target_compile_options(${PROJECT_NAME} PRIVATE /volatile:iso /Zc:__cplusplus)

  message(STATUS "Using MSVC dynamic runtime (/MD, /MDd)")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Sistema objetivo (Solo usar cuando se vaya a hacer cross-compiling)
# set(CMAKE_SYSTEM_NAME Windows)
# set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Establecer estándar C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Establecer nombre del exe igual que el nombre de la carpeta del proyecto
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME CACHE)
project(${PROJECT_NAME})

# Activa la salida de los comandos de depuración durante la generación
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Información de compilación
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

# ===============================
# Configuración del proyecto (código fuente propio)
# ==============================

# Encontrar todos los archivos .cpp, .h y .hpp
file(GLOB_RECURSE SOURCES   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")


# ===============================
# Librerías externas
# ===============================
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)                                             # menos salida en consola
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")                 # Evitar que FetchContent vuelva a actualizar el repositorio cada vez
set(EXTERNAL_LIB_PATH "${CMAKE_SOURCE_DIR}/_external")                  # Establece donde descargar las librerías externas
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/toolchains") # Añadir la carpeta toolchains/

# Libreria asio network ( incluir ANTES DE windows.h )
option(USE_ASIO "Compilar con soporte para asio" ON)         # Activar/desactivar librería
if(USE_ASIO)
  include(lib-asionetwork)                                   # Ejecuta el archivo toolchains/lib-asionetwork.cmake
  add_definitions(-DUSE_ASIO=1)                              # Definición para usar en el código C++
endif()

# Librería miniaudio
option(USE_MINIAUDIO "Compilar con soporte para Miniaudio" ON)  # Activar/desactivar librería
if(USE_MINIAUDIO)
  include(lib-miniaudio)                                     # Ejecuta el archivo toolchains/lib-miniaudio.cmake
  add_definitions(-DUSE_MINIAUDIO=1)                         # Definición para usar en el código C++
endif()

# Librería de IMGUI (gráficos, ventanas)
option(USE_IMGUI "Compilar con soporte para IMGUI" ON)       # Activar/desactivar librería
if(USE_IMGUI)
  include(lib-imgui)                                         # Ejecuta el archivo toolchains/lib-imgui.cmake
  add_definitions(-DUSE_IMGUI=1)                             # Definición para usar en el código C++
endif()

# Libreria json
option(USE_JSON "Compilar con soporte para JSON" ON)         # Activar/desactivar librería
if(USE_JSON)
  include(lib-json)                                          # Ejecuta el archivo toolchains/lib-json.cmake
  add_definitions(-DUSE_JSON=1)                              # Definición para usar en el código C++
endif()

# Libreria stb
option(USE_STB "Compilar con soporte para STB" ON)          # Activar/desactivar librería
if(USE_STB)
  include(lib-stb)                                           # Ejecuta el archivo toolchains/lib-json.cmake
  add_definitions(-DUSE_STB=1)                               # Definición para usar en el código C++
endif()

# Librería TTS
option(USE_TTS "Compilar con soporte para TTS" ON)
if(USE_TTS)
  include(lib-sherpaonnx)    # Motor de Inferencia ONNX
  add_definitions(-DUSE_TTS=1) 
endif()

    
# ----------- Librerías no probadas aún:
    
# Librería daisySP
option(USE_DAISYSP "Compilar con soporte para daisySP" OFF)      # Activar/desactivar librería
if(USE_DAISYSP)
include(lib-daisysp)                                            # Ejecuta el archivo toolchains/lib-daisysp.cmake
add_definitions(-DUSE_DAISYSP=1)                                # Definición para usar en el código C++
endif()

# ===============================
# Crear el ejecutable
# ===============================

# Crear el ejecutable principal
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# Rutas de archivos de cabecera del proyecto (archivos .h propios)
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/resources
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# ===============================
# Configuración específica del compilador/SO
# ===============================

if (WIN32)

  # Linkado de librerías del sistema Windows
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ole32       # AudioDrivers: Component Object Model (COM) y OLE
    oleaut32    # AudioDrivers: Automatización OLE
    uuid        # Soporte para GUIDs y UUIDs
    version     # Información de versión de archivos ejecutables
    dwmapi      # Integración con el gestor de ventanas DWM
   )

  # Propiedades para sistema Windows
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    WIN32_EXECUTABLE $<CONFIG:Release>                    # No usar consola en Release
  )

  # Definiciones para compatibilidad
  target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _WIN32_WINNT=0x0601       # Windows 7 o superior
    WINVER=0x0601             # Windows 7 o superior
    NTDDI_VERSION=0x06010000  # Windows 7 o superior
    _CRT_SECURE_NO_WARNINGS   # Desactivar advertencias de funciones inseguras
    _SCL_SECURE_NO_WARNINGS   # Desactivar advertencias de seguridad en STL
    NOMINMAX                  # Evitar definición de macros min y max
    WIN32_LEAN_AND_MEAN       # Excluir APIs innecesarias de Windows.h
  )

  if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE 
      $<$<CONFIG:Release>:-mwindows>
    )
  endif()

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Código de instrumentación ASan para desbordamientos de memoria
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
          # -fsanitize=address      # NECESITA LA DLL libclang_rt.asan_dynamic-x86_64.dll
          -fno-omit-frame-pointer
        >
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
          # -fsanitize=address      # NECESITA LA DLL libclang_rt.asan_dynamic-x86_64.dll
        >
    )
  endif()

endif()


# Configuración específica para MSVC (IDE Visual Studio)
if(MSVC)
  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
    /W3                       # Nivel de advertencia 3
    /EHsc                     # Manejo de excepciones en C++
    /permissive
    $<$<CONFIG:Debug>:
      /ZI                     # Información de depuración
      /Od                     # Desactivar optimizaciones en Debug
    >  
    $<$<CONFIG:Release>:/O2>  # Optimización para velocidad en Release
  )

  # Usar main normal, subsistema windows
  target_link_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
    "/ENTRY:mainCRTStartup" # Usar el main normal aunque sea una App de Windows
  )

  # Especifica el proyecto principal
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

  # Agrupar archivos de cabecera, fuente y recursos en carpetas
  source_group("Include"    FILES ${HEADERS})
  source_group("Source"     FILES ${SOURCES})
  source_group("Resources"  FILES ${RESOURCES})


  # Configuración específica para GCC/MinGW y Clang
elseif (MINGW)

  # Mensajes de información sobre el compilador
  message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "Compiler path: ${CMAKE_CXX_COMPILER}")

  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall                      # Habilitar todas las advertencias
    -Wextra                    # Habilitar advertencias adicionales
    -Wpedantic                 # Habilitar advertencias estrictas de estándar
    -Wshadow                   # Advertir sobre variables que ocultan otras
    -Wuninitialized            # Advertir sobre variables no inicializadas
    -Wformat=2                 # Advertir sobre problemas de formato en printf/scanf
    -Wunused-parameter         # Advertir de variables que no se usan

    # Depuración y optimización según la configuración
    $<$<NOT:$<CONFIG:Release>>:
      -g    # Información de depuración en Debug
      -O0   # Sin optimizaciones en Debug
    >   
    $<$<CONFIG:Release>:
      -O2   # Optimización para velocidad en Release
    >
  )

  # Linkado estático de librerías de std c++
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    LINK_FLAGS "-static -static-libgcc -static-libstdc++"
  )

  # Librerías de sistema 
  target_link_libraries(${PROJECT_NAME} PRIVATE
    winpthread  # Hilos en Windows
    kernel32    # Funciones básicas de Windows
    comdlg32    # Diálogos comunes de Windows
  )

  # Silenciar advertencia de variable no usada
  if(CMAKE_TOOLCHAIN_FILE)
      message(STATUS "Utilizando toolchain: ${CMAKE_TOOLCHAIN_FILE}")
  endif() 

endif()


# ==============================
# Enlazar con las librerías externas
# ==============================

# Se enlazan automáticamente si están activas
target_link_libraries(${PROJECT_NAME} PRIVATE 
  $<$<BOOL:${USE_ASIO}>:asio_lib>              # Librería creada con Asio + Winsock2 (Windows) 
  $<$<BOOL:${USE_IMGUI}>:imgui_lib>            # Librería creada con ImGui + GLFW + OpenGL
  $<$<BOOL:${USE_JSON}>:nlohmann_json>         # Librería creada con nlohmann/json
  $<$<BOOL:${USE_MINIAUDIO}>:miniaudio_lib>    # Librería creada con Miniaudio + winmm (Windows)
  $<$<BOOL:${USE_STB}>:stb_lib>                # Librería creada con STB
  $<$<BOOL:${USE_TTS}>:sherpa_lib>             # Librería creada con motor TTS (Sherpa)
  $<$<BOOL:${USE_DAISYSP}>:daisysp_lib>        # Librería creada con DaisySP (DSP)
)

# Copiar librerías dinámicas de TTS
if(USE_TTS)
  copy_sherpa_assets()
endif()

# ===============================
# Configuración post-build
# ===============================

# Copia todo el contenido de la carpeta 'dependencies' a la carpeta del binario
set(DEPS_DIR "dependencies")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/${DEPS_DIR}/"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  COMMENT "Copying files in ${DEPS_DIR} folder..."
)

# Configurar carpetas de salida del ejecutable y librerías
set_target_properties(${PROJECT_NAME} PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida del ejecutable
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías estáticas
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías dinámicas
)
