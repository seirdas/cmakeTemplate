
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

# Identificar arquitectura de compilación (después de project() )
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Targeting 64-bit architecture.")
    set(ARCH_NAME "x64" CACHE STRING "Target Architecture")
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Targeting 32-bit architecture.")
    set(ARCH_NAME "x86" CACHE STRING "Target Architecture")
endif()

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

include(FetchContent)                                                   # Activar módulo de descarga
set(FETCHCONTENT_QUIET OFF)                                             # Menos salida en consola
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")                 # Evitar que FetchContent vuelva a actualizar el repositorio cada vez
set(EXTERNAL_LIB_PATH "${CMAKE_SOURCE_DIR}/_external")                  # Establece donde descargar las librerías externas
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/toolchains") # Añadir la carpeta toolchains/ de librerías externas

set(EXTERNAL_LIBS "")     # Variable para almacenar las librerías activas
set(LIB_DEFINITIONS "")   # Variable para almacenar definiciones para usar en el código C++


# -----------------
# Activación de librerías (activar según convenga)
# -----------------

# Macro de factorización
#   - ${ARGN}:    Lista de archivos a incluir mediante include()
macro(import_dependency DEF_NAME ENABLE LINK_LIB)
    if(${ENABLE})   # Variable que activa/desactiva (ON/OFF)

        set(LIB_VERSION 0.0.0)  # Esperamos que la complete el toolchain de la librería

        # Incluye todos los argumentos extra (los toolchain)
        foreach(TOOLCHAIN_FILE ${ARGN})
            include(${TOOLCHAIN_FILE})
        endforeach()
        
        # Guarda el nombre de la librería creada en EXTERNAL_LIBS (puede ser "" si no aplica)
        if(NOT "${LINK_LIB}" STREQUAL "")
            list(APPEND EXTERNAL_LIBS ${LINK_LIB})
        endif()
        
        # Añade definiciones para usar en código C++ y cmake
        set(${DEF_NAME} 1)
        list(APPEND LIB_DEFINITIONS ${DEF_NAME}=1)                        # Librería activa     (e.g. #ifdef ASIO)
        list(APPEND LIB_DEFINITIONS ${DEF_NAME}_VERSION=${LIB_VERSION})   # Versión de librería (e.g. #ifdef ASIO_VERSION)

    endif()
endmacro()

# Librerías externas
import_dependency( SHERPA           ON     sherpa_lib         audio/lib-sherpaonnx audio/tts-assets) # Tiene 2 toolchains
import_dependency( MINIAUDIO        ON     miniaudio_lib      audio/lib-miniaudio)
import_dependency( DAISYSP          OFF    daisysp_lib        audio/lib-daisysp)
import_dependency( ASIO             ON     asio_lib           net/lib-asionetwork)
import_dependency( IMGUI            ON     imgui_lib          gui/lib-imgui)
import_dependency( STB              ON     stb_lib            gui/lib-stb)
import_dependency( JSON             ON     nlohmann_json      file/lib-json)
import_dependency( XML              ON     tinyxml2_lib       file/lib-xml)
import_dependency( FASTDDS          OFF    fastdds_lib        dds/lib-fastdds)    # Necesita instalar Fastdds 3.2.2, OpenSSL y Java JDK
import_dependency( CYCLONEDDSCXX    OFF    cycloneddscxx_lib  dds/lib-cycloneddscxx)
import_dependency( OPENDDS          OFF    opendds_lib        dds/lib-opendds)

# Utilidades
import_dependency( NINJA            ON     ""                 tools/utils-ninja)    # Necesita al menos una configuración correcta previa
import_dependency( DOXYGEN          ON     ""                 tools/utils-doxygen)



# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# ===============================
# CREAR EL EJECUTABLE
# ===============================
# ¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡

# Crear el ejecutable principal
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# Rutas de archivos de cabecera del proyecto (archivos .h propios)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/resources
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# ===============================
# Configuración específica del SO
# ===============================

if (WIN32)

  # Linkado de librerías del sistema Windows
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ole32       # AudioDrivers: Component Object Model (COM) y OLE
    oleaut32    # AudioDrivers: Automatización OLE
    uuid        # Soporte para GUIDs y UUIDs
    version     # Información de versión de archivos ejecutables
    dwmapi      # Integración con el gestor de ventanas DWM
    ws2_32      # Winsock2: sockets UDP (TotalMix OSC, Symetrix)
    user32      # MessageBoxA
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


# ===============================
# Configuración específica del compilador
# ===============================
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

  # Flag para mejorar la compatibilidad de headers antiguos
  target_compile_options(${PROJECT_NAME} PRIVATE /volatile:iso /Zc:__cplusplus)

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

# Enlazar las librerías externas activas creadas
target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${EXTERNAL_LIBS}
)

# Inyectar las definiciones acumuladas de librerías a C++
target_compile_definitions(${PROJECT_NAME} PRIVATE 
  ${LIB_DEFINITIONS}
)

# Copiar librerías dinámicas de TTS
if(SHERPA)
  configure_sherpa_deps()
  link_tts_assets()
endif()

# Copiar librerías dinámicas de Cyclone DDSCXX
if(CYCLONEDDSCXX)
  configure_cyclonedds_dlls()
endif()


# ===============================
# Configuración post-build
# ===============================

# Copia todo el contenido de la carpeta 'dependencies' a la carpeta del binario
set(DEPS_DIR "dependencies")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "---- Copying files from ${DEPS_DIR} folder..."
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/${DEPS_DIR}/"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Configurar carpetas de salida del ejecutable y librerías
set_target_properties(${PROJECT_NAME} PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<IF:$<PLATFORM_ID:Windows>,Win,Linux>-${ARCH_NAME}-$<CONFIG>" # Carpeta de salida del ejecutable
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<IF:$<PLATFORM_ID:Windows>,Win,Linux>-${ARCH_NAME}-$<CONFIG>" # Carpeta de salida de librerías estáticas
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<IF:$<PLATFORM_ID:Windows>,Win,Linux>-${ARCH_NAME}-$<CONFIG>" # Carpeta de salida de librerías dinámicas
)

# Generar Doxygen
if (USE_DOXYGEN)
    generate_doxygen()
endif()
