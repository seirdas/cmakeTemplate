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

#todo