# -------------------------------
# Librería de Fast-DDS con dependencias
# genera la librería fastdds_lib
# -------------------------------

# Opciones de compilación de Fast-DDS
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(STRICT_REALTIME     OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
set(FORCE_INTERNAL_FASTCDR ON CACHE BOOL "" FORCE)              # Forzar a usar FastCDR descargado
set(FASTRTPS_INSTALLER_LOC "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)   # Para localizar deps internas


# =======================
# Lógica de descarga
# =======================


# =======================
# Dependencia: FastCDR
# =======================

if (EXISTS "${EXTERNAL_LIB_PATH}/fastcdr_src/.git")
  message(STATUS "Using local fastcdr source")
  set(FETCHCONTENT_SOURCE_DIR_FASTCDR
      "${EXTERNAL_LIB_PATH}/fastcdr_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    fastcdr
    GIT_REPOSITORY "https://github.com/eProsima/Fast-CDR"
    GIT_TAG        v1.0.29
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastcdr_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastcdr)

if (EXISTS "${EXTERNAL_LIB_PATH}/foomemory_src/.git")
  message(STATUS "Using local foomemory source")
  set(FETCHCONTENT_SOURCE_DIR_FOOMEMORY
      "${EXTERNAL_LIB_PATH}/foomemory_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    foomemory
    GIT_REPOSITORY "https://github.com/foonathan/memory"
    GIT_TAG        v0.7-3
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/foomemory_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(foomemory)

if (EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.git")
  message(STATUS "Using local foomemory source")
  set(FETCHCONTENT_SOURCE_DIR_FASTDDS
      "${EXTERNAL_LIB_PATH}/fastdds_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    foomemory
    GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS"
    GIT_TAG        v3.2.1
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastdds_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(foomemory)
