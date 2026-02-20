
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

# Activa la salida de los comandos de depuración durante la generación
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
  # Definiciones de compatibilidad
  add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS # Desactivar advertencias de funciones inseguras
      _SCL_SECURE_NO_WARNINGS # Desactivar advertencias de seguridad en STL
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


# Libreria asio network ( ANTES DE windows.h )
option(USE_ASIO "Compilar con soporte para asio" ON)            # Activar/desactivar librería
if(USE_ASIO)
    include(lib-asionetwork)                                    # Ejecuta el archivo toolchains/lib-asionetwork.cmake
    add_definitions(-DUSE_ASIO=1)                               # Definición para usar en el código C++
endif()

# Librería de JUCE (audio)
option(USE_JUCE "Compilar con soporte para JUCE DSP" OFF)       # Activar/desactivar librería
if(USE_JUCE)
    include(lib-juce)                                           # Ejecuta el archivo toolchains/lib-JUCE.cmake
    add_definitions(-DUSE_JUCE=1)                               # Definición para usar en el código C++
endif()

# Librería miniaudio
option(USE_MINIAUDIO "Compilar con soporte para Miniaudio" OFF)  # Activar/desactivar librería
if(USE_MINIAUDIO)
    include(lib-miniaudio)                                      # Ejecuta el archivo toolchains/lib-miniaudio.cmake
    add_definitions(-DUSE_MINIAUDIO=1)                          # Definición para usar en el código C++
endif()

# Librería daisySP
option(USE_DAISYSP "Compilar con soporte para daisySP" OFF)      # Activar/desactivar librería
if(USE_DAISYSP)
    include(lib-daisysp)                                        # Ejecuta el archivo toolchains/lib-daisysp.cmake
    add_definitions(-DUSE_DAISYSP=1)                            # Definición para usar en el código C++
endif()

# Librería de IMGUI (gráficos, ventanas)
option(USE_IMGUI "Compilar con soporte para IMGUI" ON)          # Activar/desactivar librería
if(USE_IMGUI)
    include(lib-imgui)                                          # Ejecuta el archivo toolchains/lib-imgui.cmake
    add_definitions(-DUSE_IMGUI=1)                              # Definición para usar en el código C++
endif()

# Libreria json
option(USE_JSON "Compilar con soporte para JSON" ON)            # Activar/desactivar librería
if(USE_JSON)
    include(lib-json)                                           # Ejecuta el archivo toolchains/lib-json.cmake
    add_definitions(-DUSE_JSON=1)                               # Definición para usar en el código C++
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
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ole32       # AudioDrivers: Component Object Model (COM) y OLE
    oleaut32    # AudioDrivers: Automatización OLE
    uuid        # Soporte para GUIDs y UUIDs
    version     # Información de versión de archivos ejecutables
    dwmapi      # Integración con el gestor de ventanas DWM
   )

  set_target_properties(${PROJECT_NAME} PROPERTIES 
    WIN32_EXECUTABLE $<CONFIG:Release>      # No usar consola en Release
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" # Usar el main normal aunque sea una App de Windows
  )

  # Definir _WIN32_WINNT para el ejecutable
  target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _WIN32_WINNT=0x0601
    WINVER=0x0601
    NTDDI_VERSION=0x06010000
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
  
  # Configuración para depuración en Visual Studio: establecer el ejecutable y el directorio de trabajo
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

  # Definiciones para compatibilidad con Windows 7 o superior (Windows 7 = 0x0601)
  target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _WIN32_WINNT=0x0601       # Windows 7 o superior
    WINVER=0x0601             # Windows 7 o superior
    NTDDI_VERSION=0x06010000  # Windows 7 o superior
  )


# Configuración específica para GCC/MinGW
elseif(MINGW)
  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
      -Wall                      # Habilitar todas las advertencias
      -Wextra                    # Habilitar advertencias adicionales
      -Wpedantic                 # Habilitar advertencias estrictas de estándar
      -Wshadow                   # Advertir sobre variables que ocultan otras
      -Wuninitialized            # Advertir sobre variables no inicializadas
      -Wformat=2                 # Advertir sobre problemas de formato en printf/scanf

      # Depuración y optimización según la configuración
      $<$<NOT:$<CONFIG:Release>>:-g>   # Información de depuración en Debug
      $<$<NOT:$<CONFIG:Release>>:-O0>  # Sin optimizaciones en Debug
      $<$<CONFIG:Release>:-O2>         # Optimización para velocidad en Release
  )


  set_target_properties(${PROJECT_NAME} PROPERTIES 
    LINK_FLAGS "-static -static-libgcc -static-libstdc++"
  )

  # Librerías de sistema 
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -static
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

target_link_libraries(${PROJECT_NAME} PRIVATE 
  $<$<BOOL:${USE_ASIO}>:asio_lib>                   # Librería creada con Asio + Winsock2 (Windows) 
  $<$<BOOL:${USE_IMGUI}>:imgui_lib>                 # Librería creada con ImGui + GLFW + OpenGL
  $<$<BOOL:${USE_JUCE}>:juce::juce_dsp>
  $<$<BOOL:${USE_JUCE}>:juce::juce_audio_devices>
  $<$<BOOL:${USE_JSON}>:nlohmann_json>              # Librería creada con nlohmann/json
  $<$<BOOL:${USE_MINIAUDIO}>:miniaudio_lib>         # Librería creada con Miniaudio + winmm (Windows)
  $<$<BOOL:${USE_DAISYSP}>:daisysp_lib>             # Librería creada con DaisySP (DSP)
)

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
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida del ejecutable
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías estáticas
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>" # Carpeta de salida de librerías dinámicas
)
