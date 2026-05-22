# -------------------------------
# Librería de opendds con dependencias
#  - genera la librería opendds
#  - genera código fuente desde archivos .idl
# -------------------------------

include_guard(GLOBAL)


# -------------------------------
# ACE+TAO
# -------------------------------

message(STATUS "Fetching ace_tao library...")
set(ACETAO_SRC_DIR   "${EXTERNAL_LIB_PATH}/acetao_src"  CACHE PATH "Fuentes de ACE/TAO")
set(OPENDDS_MPC_GIT ON CACHE BOOL "Descargar MPC vía Git para OpenDDS" FORCE)
# Usa la librería ya descargada en external/ si existe
if (EXISTS "${ACETAO_SRC_DIR}/.git")
  message(STATUS "Library 'ace_tao' found locally at: '${ACETAO_SRC_DIR}'")
  set(FETCHCONTENT_SOURCE_DIR_ACE_TAO
      "${ACETAO_SRC_DIR}"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    ace_tao
    GIT_REPOSITORY "https://github.com/DOCGroup/ACE_TAO"
    GIT_TAG        "ACE+TAO-8_0_5"
    SOURCE_DIR     "${ACETAO_SRC_DIR}"
    GIT_SHALLOW      TRUE        # habilita --depth 1
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(ace_tao)


# -------------------------------
# OpenDDS
# -------------------------------

set(OPENDDS_SRC_DIR         "${EXTERNAL_LIB_PATH}/opendds_src"  CACHE PATH "Fuentes de ACE/TAO")

set(OPENDDS_BUILD_TESTS     OFF                 CACHE BOOL "No compilar tests de OpenDDS"        FORCE)
set(OPENDDS_BUILD_EXAMPLES  OFF                 CACHE BOOL "No compilar ejemplos de OpenDDS"     FORCE)

if (EXISTS "${OPENDDS_SRC_DIR}/.git")
  message(STATUS "Library 'ace_tao' found locally at: '${ACETAO_SRC_DIR}'")
  set(FETCHCONTENT_SOURCE_DIR_OPENDDS
      "${OPENDDS_SRC_DIR}"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    opendds
    GIT_REPOSITORY   "https://github.com/OpenDDS/OpenDDS.git"
    GIT_TAG          v3.33.0
    SOURCE_DIR       "${OPENDDS_SRC_DIR}"   # ← persistente en _external/opendds_src/
    GIT_SHALLOW      TRUE
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL TRUE
)

FetchContent_MakeAvailable(opendds)

# Guardar rutas para los pasos de generación IDL
set(OPENDDS_SOURCE_DIR "${opendds_SOURCE_DIR}" CACHE INTERNAL "OpenDDS source dir")
set(OPENDDS_BINARY_DIR "${opendds_BINARY_DIR}" CACHE INTERNAL "OpenDDS binary dir")

message(STATUS "[OpenDDS] Source: ${OPENDDS_SOURCE_DIR}")
message(STATUS "[OpenDDS] Binary: ${OPENDDS_BINARY_DIR}")


# -------------------------------
# IDL Generator
# -------------------------------
set(OPENDDS_IDL_SOURCE_DIR    "${CMAKE_SOURCE_DIR}/IDL")
set(OPENDDS_IDL_GENERATED_DIR "${CMAKE_SOURCE_DIR}/IDL/generated_opendds")

file(GLOB OPENDDS_IDL_FILES "${OPENDDS_IDL_SOURCE_DIR}/*.idl")
message(STATUS "OPENDDS_IDL_FILES:${OPENDDS_IDL_FILES}")

# Solo si hay archivos IDL, creamos su librería
if(OPENDDS_IDL_FILES)
    # 1. Creamos una librería ESTÁTICA para compilar los .cpp generados
    add_library(mis_mensajes_idl STATIC "") 
    
    # 2. La macro hace su magia: genera los .cpp/.hpp y los asigna a 'mis_mensajes_idl'
    opendds_target_sources(mis_mensajes_idl ${OPENDDS_IDL_FILES})

    # 3. La librería de mensajes necesita de OpenDDS para poder compilarse a sí misma
    target_link_libraries(mis_mensajes_idl PUBLIC OpenDDS::Dcps)
    
    # 4. Exponemos la ruta donde se han generado los .hpp (por defecto CMAKE_CURRENT_BINARY_DIR)
    # al target, para que quien use 'mis_mensajes_idl' sepa dónde encontrar los headers.
    target_include_directories(mis_mensajes_idl PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Corregir rutas para el generador de código de OpenDDS
set(ENV{DDS_ROOT} "${OPENDDS_SRC_DIR}")
set(ENV{ACE_ROOT} "${ACETAO_SRC_DIR}/ACE")
set(ENV{TAO_ROOT} "${ACETAO_SRC_DIR}/TAO")

# Obligar a CMake a usar rutas absolutas para las plantillas
set(opendds_idl_templates "${OPENDDS_SRC_DIR}/dds/idl")


# -------------------------------
# Creación de librería
# -------------------------------

add_library(opendds_lib INTERFACE)

# Agrupamos las carpetas de inclusión
target_include_directories(opendds_lib INTERFACE 
    ${OPENDDS_IDL_GENERATED_DIR}  # Donde se generan los .h del IDL
    ${OPENDDS_SRC_DIR}
    ${ACETAO_SRC_DIR}
    ${ACETAO_SRC_DIR}/TAO
)

# Agrupamos las dependencias core de OpenDDS
target_link_libraries(opendds_lib INTERFACE 
    OpenDDS::Dcps
    OpenDDS::Rtps
    OpenDDS::Rtps_Udp
)

# Si se generó la librería de los IDL, la enganchamos al paquete final
if(OPENDDS_IDL_FILES)
    target_link_libraries(opendds_lib INTERFACE mis_mensajes_idl)
endif()