# -------------------------------
# Librería de Fast-DDS con dependencias
# genera la librería fastdds
# -------------------------------

# Opciones de compilación de Fast-DDS
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(STRICT_REALTIME     OFF CACHE BOOL "" FORCE)
set(FORCE_INTERNAL_FASTCDR ON CACHE BOOL "" FORCE)              # Forzar a usar FastCDR descargado
set(FASTRTPS_INSTALLER_LOC "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)   # Para localizar deps internas

# =======================
# Dependencia: FastCDR
# =======================
message(STATUS "[DDS] Fetching fastcdr library...")
if (EXISTS "${EXTERNAL_LIB_PATH}/fastcdr_src/.git")
  message(STATUS "Library 'fastcdr' found locally at: '${EXTERNAL_LIB_PATH}/fastcdr_src'")
  set(FETCHCONTENT_SOURCE_DIR_FASTCDR
      "${EXTERNAL_LIB_PATH}/fastcdr_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    fastcdr
    GIT_REPOSITORY "https://github.com/eProsima/Fast-CDR"
    GIT_TAG        v2.2.1
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastcdr_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastcdr)
set(fastcdr_DIR "${fastcdr_BINARY_DIR}/cmake" CACHE PATH "" FORCE)

# =======================
# Dependencia: foonathan_memory
# =======================
set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
message(STATUS "[DDS] Fetching foonathan_memory library...")
if (EXISTS "${EXTERNAL_LIB_PATH}/foonathan_memory_src/.git")
  message(STATUS "Library 'foonathan_memory' found locally at: '${EXTERNAL_LIB_PATH}/foonathan_memory_src'")
  set(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY
      "${EXTERNAL_LIB_PATH}/foonathan_memory_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY "https://github.com/foonathan/memory"
    GIT_TAG        v0.7-3
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/foonathan_memory_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(foonathan_memory)

# =======================
# Core: FastDDS
# =======================
message(STATUS "[DDS] Fetching fastdds library...")
if (EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.git")
  message(STATUS "Library 'fastdds' found locally at: '${EXTERNAL_LIB_PATH}/fastdds_src'")
  set(FETCHCONTENT_SOURCE_DIR_FASTDDS
      "${EXTERNAL_LIB_PATH}/fastdds_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    fastdds
    GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS"
    GIT_TAG        v3.2.1
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastdds_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastdds)

# =========================================================
# El target 'fastdds' ya es exportado automáticamente por eProsima.
# =========================================================