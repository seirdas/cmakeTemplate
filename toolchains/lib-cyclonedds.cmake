# -------------------------------
# Librería CycloneDDS
# Target final: CycloneDDS::ddsc (o ddsc)
# -------------------------------

include(FetchContent)

message(STATUS "Fetching CycloneDDS...")

# =======================
# Opciones de compilación
# =======================

# Compilación estática
set(CMAKE_CROSSCOMPILING ON CACHE INTERNAL "" FORCE)
if(NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME} CACHE INTERNAL "" FORCE)
endif()
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)

set(ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
set(CYCLONEDDS_INSTALL_C_HEADERS    OFF CACHE INTERNAL "")
set(BUILD_IDLC      ON   CACHE INTERNAL "")  # ON: necesario para compilar archivos .idl
set(ENABLE_SSL      OFF  CACHE INTERNAL "")
set(ENABLE_SHM      OFF  CACHE INTERNAL "")
set(ENABLE_ICEORYX  OFF  CACHE INTERNAL "")
set(BUILD_EXAMPLES  OFF  CACHE INTERNAL "")
set(BUILD_TESTING   OFF  CACHE INTERNAL "")
set(BUILD_DDSPERF   OFF  CACHE INTERNAL "")
set(ENABLE_SECURITY OFF CACHE INTERNAL "")

# =======================
# Descarga
# =======================
set(CYCLONE_SRC_DIR "${EXTERNAL_LIB_PATH}/cyclonedds_src")
if(EXISTS "${CYCLONE_SRC_DIR}/.git")
    message(STATUS "Using local CycloneDDS source at ${CYCLONE_SRC_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_CYCLONEDDS "${CYCLONE_SRC_DIR}" CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    cyclonedds
    GIT_REPOSITORY https://github.com/eclipse-cyclonedds/cyclonedds.git
    GIT_TAG        11.0.1      # Usa un tag concreto para builds reproducibles
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${CYCLONE_SRC_DIR}"
    EXCLUDE_FROM_ALL TRUE
)
set(CMAKE_SKIP_INSTALL_RULES TRUE) 
FetchContent_MakeAvailable(cyclonedds)
set(CMAKE_SKIP_INSTALL_RULES FALSE)
if(TARGET ddsc)
    target_include_directories(ddsc PRIVATE 
        "${CYCLONE_SRC_DIR}/src/core/cdr/include"
        "${CYCLONE_SRC_DIR}/src/core/include"
        "${CYCLONE_SRC_DIR}/src/core/ddsi/include"
        "${CYCLONE_SRC_DIR}/src/core/ddsc/include"
        "${CYCLONE_SRC_DIR}/src/core/ddsc/src"
        "${CYCLONE_SRC_DIR}/src/core/ddsi/src"
        "${CYCLONE_SRC_DIR}/src/ddsrt/include"
        "${CYCLONE_SRC_DIR}/src/ddsrt/src"
        "${CYCLONE_SRC_DIR}/src/core/ddsi/include"
    )
    target_include_directories(ddsc PRIVATE 
        ${CMAKE_BINARY_DIR}/_deps/cyclonedds-build/include # <--- ¡IMPORTANTE! Aquí están los generados
    )
endif()

# Omitir warnings de la librería
set(CYCLONE_TARGETS ddsc idlc )
foreach (tgt ${CYCLONE_TARGETS})
    if (MSVC)
        # --- Configuración para MSVC (Visual Studio) ---
        target_compile_options(${tgt} PRIVATE
            /W0            # Nivel de advertencia 0 (silencio total)
            /wd4244        # double a float
            /wd4305        # truncamiento de constantes
            /wd4267        # size_t a int
            /external:W0   # (CMake 3.22+) Silencia cabeceras externas
        )
    else()
        # --- Configuración para GCC / Clang / MinGW ---
        target_compile_options(${tgt} PRIVATE
            -w             # Suprime todos los warnings
            -Wno-conversion
            -Wno-sign-compare
            -Wno-unused-parameter
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -Wno-shadow
            -Wno-implicit-function-declaration
            -Wno-int-conversion
        )
    endif()
endforeach()

# Crear alias de namespace (FetchContent expone targets sin namespace)
if(TARGET ddsc AND NOT TARGET CycloneDDS::ddsc)
    add_library(CycloneDDS::ddsc ALIAS ddsc)
endif()
if(TARGET idlc AND NOT TARGET CycloneDDS::idlc)
    add_executable(CycloneDDS::idlc ALIAS idlc)
endif()


# =======================
# Compilación de archivos IDL (en modo .c)
# idlc_generate crea automáticamente un target con los .c/.h generados.
# =======================

file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")

if(IDL_FILES)
      message(STATUS "Archivos IDL encontrados: ${IDL_FILES}")

      # Generar el "código" c y h a partir de los .idl de la carpeta IDL
      idlc_generate(
        TARGET idl_generated_lib
        FILES  ${IDL_FILES}
      )
      add_dependencies(idl_generated_lib idlc)

      # Silenciar los warnings de los enums en el código generado (MSVC 2026)
      if(MSVC)
          target_compile_options(idl_generated_lib INTERFACE /wd5286 /wd5287 /W0)
      endif()

      # Añade el .h generado al proyecto
      target_include_directories(idl_generated_lib 
        INTERFACE 
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      )
  else()
      message(WARNING "No se encontraron archivos .idl en la carpeta IDL/")
endif()

