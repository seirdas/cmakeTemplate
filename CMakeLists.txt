
# ==============================
# CMakeLists.txt - Configuración del proyecto CMake
# ==============================

# CMake minimum version and project setup
cmake_minimum_required(VERSION 3.15)

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

# Variable para almacenar definiciones para usar en el código C++
set(CXX_DEFINITIONS "")

# Establecer nombre del exe igual que el nombre de la carpeta del proyecto
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME CACHE)
project(${PROJECT_NAME})

# No uso este APP_NAME en C++ por si cambia el nombre del exe después de compilar
# list(APPEND CXX_DEFINITIONS APP_NAME="${PROJECT_NAME}")


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

# Carpeta del src y del include que será compilada con compatibilidad CLI .NET
set(CLINET_FOLDER "CLI.NET")

# Sources:
file(GLOB_RECURSE SOURCES     CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
# Separar los cpps que necesitan la compatibilidad .NET (CLI, /clr)
list(FILTER SOURCES EXCLUDE REGEX ".*src/${CLINET_FOLDER}/.*")

# Headers:
file(GLOB_RECURSE HEADERS     CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
list(FILTER HEADERS EXCLUDE REGEX ".*include/${CLINET_FOLDER}/.*")

if(MSVC)
    # Separar los cpps que necesitan la compatibilidad .NET (CLI, /clr)
    file(GLOB_RECURSE CLI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/${CLINET_FOLDER}/*.cpp")
    file(GLOB_RECURSE CLI_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/${CLINET_FOLDER}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/${CLINET_FOLDER}/*.hpp")
    
    # Aplicar propiedades de MSVC/CLR
    set_source_files_properties(${CLI_SOURCES} PROPERTIES COMPILE_FLAGS "/clr /EHa /Zi")
else()
    # Si NO es MSVC (MinGW/Clang), eliminamos los archivos CLI de las listas
    set(CLI_SOURCES "")
    set(CLI_HEADERS "")
endif()

# Resources:
file(GLOB_RECURSE RESOURCES   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")


# ==============================
# Crear librería estática con código fuente de CLI.NET
# ==============================

set(HAVE_CLI_MANAGED OFF)

if (MSVC AND CLI_SOURCES)
    add_library(CliWrapper OBJECT ${CLI_SOURCES} ${CLI_HEADERS})
    target_include_directories(CliWrapper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Target completo managed - MSBuild pondrá CLRSupport=true en todo el .vcxproj de CliWrapper
    set_property(TARGET CliWrapper PROPERTY COMMON_LANGUAGE_RUNTIME "")

    # Flags propias, SIN /RTC1, /ZI ni /MP (incompatibles con /clr)
    target_compile_options(CliWrapper PRIVATE
        /EHa
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:Release>:/O2 /Oi>
    )

    # Toma las dlls correspondientes a la arquitectura (x86/x64)
    set(NET_DEPS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/.NET/${ARCH_NAME})
    target_compile_options(CliWrapper PRIVATE "/AI${NET_DEPS_FOLDER}")

    set(HAVE_CLI_MANAGED ON)
    message(STATUS "CLI managed sources found, target CliWrapper created")
endif()


# ===============================
# Librerías externas
# ===============================

include(FetchContent)                                                   # Activar módulo de descarga
set(FETCHCONTENT_QUIET OFF)                                             # Menos salida en consola
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")                 # Evitar que FetchContent vuelva a actualizar el repositorio cada vez
set(EXTERNAL_LIB_PATH "${CMAKE_SOURCE_DIR}/_external")                  # Establece donde descargar las librerías externas
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/toolchains") # Añadir la carpeta toolchains/ de librerías externas

set(EXTERNAL_LIBS "")     # Variable para almacenar las librerías activas

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
        list(APPEND CXX_DEFINITIONS ${DEF_NAME}=1)                        # Librería activa     (e.g. #ifdef ASIO)
        list(APPEND CXX_DEFINITIONS ${DEF_NAME}_VERSION=${LIB_VERSION})   # Versión de librería (e.g. #ifdef ASIO_VERSION)

    endif()
endmacro()

# Librerías externas
import_dependency( SHERPA           ON     sherpa_lib         tts/lib-sherpaonnx tts/tts-assets) # Tiene 2 toolchains
import_dependency( MINIAUDIO        ON     miniaudio_lib      audio/lib-miniaudio)
import_dependency( DAISYSP          OFF    daisysp_lib        audio/lib-daisysp)
import_dependency( ASIO_NETWORK     ON     asio_lib           net/lib-asionetwork)
import_dependency( IMGUILIB         ON     imgui_lib          gui/lib-imgui)
import_dependency( STB              ON     stb_lib            gui/lib-stb)
import_dependency( JSON             ON     nlohmann_json      file/lib-json)
import_dependency( TINYXML          ON     tinyxml2_lib       file/lib-xml)
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

  # No usar consola en Release o MinSizeRel
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>
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
      $<$<NOT:$<CONFIG:Debug>>:-mwindows>
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
  # =====================================================================
  # CONFIGURACIÓN PARA C++/CLI (.NET)
  # =====================================================================

  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
    /W4                       # Nivel de advertencia 4
    /w44062                   # Advertencia cuando falta un caso en switch de enum
    /EHa                     # Manejo de excepciones asíncronas en C++ (compatible con .NET)
    /MP                       # Compilación multiproceso
    # /WX                     # Tratar warnings como errores

    # Flags específicos por configuración
    $<$<CONFIG:Debug>:
      $<$<BOOL:${HAVE_CLI_MANAGED}>:/Zi>        # Símbolos de depuración (Compatible con CLI.NET)
      $<$<NOT:$<BOOL:${HAVE_CLI_MANAGED}>>:/ZI> # Símbolos de depuración (Edit and Continue)
      /Od                     # Desactivar optimizaciones
      /RTC1                   # Verificaciones en tiempo de ejecución (Stack frames / uninitialized)
    >  
    $<$<CONFIG:Release>:
      /O2                     # Optimización para velocidad máxima
      /Oi                     # Habilitar funciones intrínsecas
      /GL                     # Optimización global (Whole Program Optimization)
    >
    $<$<CONFIG:MinSizeRel>:
      /O1                     # Optimización enfocada estrictamente en tamaño mínimo
      /Oi                     # Habilitar intrínsecas
    >
    $<$<CONFIG:RelWithDebInfo>:
      /O2                     # Optimización para velocidad (igual que Release)
      /Zi                     # Generar símbolos de depuración completos (sin Edit & Continue)
      /Oi                     # Habilitar funciones intrínsecas
    >
  )

  # Flag para mejorar la compatibilidad de headers antiguos
  target_compile_options(${PROJECT_NAME} PRIVATE /volatile:iso /Zc:__cplusplus)

  # Configuración de MSVC para Windows
  target_link_options(${PROJECT_NAME} PRIVATE 
    # Aplicar /SUBSYSTEM:WINDOWS para Release y MinSizeRel
    $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/SUBSYSTEM:WINDOWS>
    
    # Aplicar /LTCG solo en Release (para optimización global)
    $<$<CONFIG:Release>:/LTCG>
    
    # Configurar punto de entrada como 'main'
    "/ENTRY:mainCRTStartup"
  )

  # Especifica el proyecto principal
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

  # Agrupar archivos de cabecera, fuente y recursos en carpetas
  source_group("Include"    FILES ${HEADERS})
  source_group("Source"     FILES ${SOURCES})
  source_group("Resources"  FILES ${RESOURCES})
  if(CLI_SOURCES)
    source_group("Source\\CLI.NET" FILES ${CLI_SOURCES})
  endif()
  if(CLI_HEADERS)
    source_group("Include\\CLI.NET" FILES ${CLI_HEADERS})
  endif()

  # Localizar las dependencias msvcp140(d) y vcruntime140d usando el path del compilador según la configuración actual (Debug o Release)
  set(MSVCP_NAME "$<IF:$<CONFIG:Debug>,msvcp140d.dll,msvcp140.dll>")
  set(VCRUNTIME_NAME "$<IF:$<CONFIG:Debug>,vcruntime140d.dll,vcruntime140.dll>")
  get_filename_component(MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  find_file(MSVCP_DLL_PATH NAMES ${MSVCP_NAME} PATHS "${MSVC_BIN_DIR}" NO_DEFAULT_PATH)
  find_file(VCRUNTIME_DLL_PATH NAMES ${VCRUNTIME_NAME} PATHS "${MSVC_BIN_DIR}" NO_DEFAULT_PATH)
  if (MSVCP_DLL_PATH)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MSVCP_DLL_PATH}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${VCRUNTIME_DLL_PATH}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
  else()
    # Fallback usando las carpetas del sistema
    if (ARCH_NAME STREQUAL "x86")
      set(SYSTEM_WINDOWS_DIR "C:/Windows/SysWOW64")
    else()
      set(SYSTEM_WINDOWS_DIR "C:/Windows/System32")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SYSTEM_WINDOWS_DIR}/${MSVCP_NAME}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SYSTEM_WINDOWS_DIR}/${VCRUNTIME_NAME}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
  endif()


elseif (MINGW)
  # Configuración específica para GCC/MinGW y Clang

  # Mensajes de información sobre el compilador
  message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "Compiler path: ${CMAKE_CXX_COMPILER}")

  # Opciones de compilación
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall                       # Habilitar la mayoría de las advertencias comunes
    -Wextra                     # Habilitar advertencias adicionales de buenas prácticas
    -Wpedantic                  # Cumplimiento estricto del estándar ISO C++
    -Wconversion                # Advertir sobre conversiones de tipos con pérdida de datos
    -Wshadow                    # Advertir cuando una variable oculta otra de un ámbito superior
    -Wformat=2                  # Advertencias estrictas para funciones printf/scanf
    -Wunused-parameter          # Advertir sobre parámetros no utilizados en funciones
    -Wnon-virtual-dtor          # Advertir si una clase polimórfica no tiene destructor virtual
    -Wold-style-cast            # Advertir sobre el uso de casts estilo C (usar C++ casts)
    -Wcast-align                # Advertir sobre punteros con alineación ineficiente
    -Wdouble-promotion          # Advertir si float se convierte implícitamente a double
    # -Werror                   # Tratar warnings como errores
    
    # Flags específicos por configuración
    $<$<CONFIG:Debug>:
      -g                      # Información de depuración
      -O0                     # Sin optimizaciones para depurar línea a línea sin saltos
    >
    $<$<CONFIG:Release>:
      -O3                     # Máxima optimización de velocidad (-O3 suele ser mejor que -O2 en GCC moderno)
      -DNDEBUG                # Desactivar asserts de cabeceras estándar
    >
    $<$<CONFIG:MinSizeRel>:
      -Os                     # Optimizar para reducir el tamaño del binario al máximo
      -DNDEBUG
    >
    $<$<CONFIG:RelWithDebInfo>:
      -O2                     # Optimización de velocidad equilibrada y segura para perfiles estables
      -g                      # Incluir símbolos de depuración independientes
    >
  )

  # Linkado estático de librerías de std
  target_link_options(${PROJECT_NAME} PRIVATE 
      $<$<CXX_COMPILER_ID:Clang>:-static -stdlib=libc++ -static-libgcc -lc++ -lc++abi -lunwind>
      $<$<NOT:$<CXX_COMPILER_ID:Clang>>:-static -static-libgcc -static-libstdc++>
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

# Enlazado con la librería estática del código CLI.NET (si aplica)
if (HAVE_CLI_MANAGED)
    target_link_libraries(${PROJECT_NAME} PRIVATE CliWrapper)
endif()


# Enlazar las librerías externas activas creadas
target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${EXTERNAL_LIBS}
)

# Inyectar las definiciones acumuladas de librerías a C++
target_compile_definitions(${PROJECT_NAME} PRIVATE 
  ${CXX_DEFINITIONS}
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

# -----------------
# Copiar dlls de CLI.NET
# -----------------

if (HAVE_CLI_MANAGED)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "---- Copying CLI.NET deps from ${NET_DEPS_FOLDER} folder..."
  )
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${NET_DEPS_FOLDER}/"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
endif()

# -----------------
# Copiar archivos puestos manualmente a la carpeta del binario
# -----------------
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
if (DOXYGEN)
    generate_doxygen()
endif()


# =======================================================================================
# Descomentar esto para pararse y debugear este archivo.
# message(FATAL_ERROR debugStop)
