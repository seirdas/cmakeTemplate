# =======================
# Dependencia: FastCDR
# =======================
message(STATUS "[DDS] Fetching fastcdr library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastcdr_src/.git")
    message(STATUS "Library 'fastcdr' found locally at: '${EXTERNAL_LIB_PATH}/fastcdr_src'")
    set(FETCHCONTENT_SOURCE_DIR_FASTCDR "${EXTERNAL_LIB_PATH}/fastcdr_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    fastcdr
    GIT_REPOSITORY "https://github.com/eProsima/Fast-CDR"
    GIT_TAG        v2.2.1
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastcdr_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastcdr)

# *** FIX: el Config está en la raíz del binary dir, NO en /cmake ***
set(fastcdr_DIR "${fastcdr_BINARY_DIR}" CACHE PATH "" FORCE)
list(PREPEND CMAKE_PREFIX_PATH "${fastcdr_BINARY_DIR}")

# =======================
# Dependencia: foonathan_memory
# =======================
set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)

message(STATUS "[DDS] Fetching foonathan_memory library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/foonathan_memory_src/.git")
    message(STATUS "Library 'foonathan_memory' found locally at: '${EXTERNAL_LIB_PATH}/foonathan_memory_src'")
    set(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY "${EXTERNAL_LIB_PATH}/foonathan_memory_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY "https://github.com/foonathan/memory"
    GIT_TAG        v0.7-3
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/foonathan_memory_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(foonathan_memory)

# *** FIX: exponer foonathan_memory al find_package interno de FastDDS ***
set(foonathan_memory_DIR "${foonathan_memory_BINARY_DIR}" CACHE PATH "" FORCE)
list(PREPEND CMAKE_PREFIX_PATH "${foonathan_memory_BINARY_DIR}")

# =======================
# Opciones de FastDDS (antes del MakeAvailable)
# =======================
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(STRICT_REALTIME     OFF CACHE BOOL "" FORCE)
# *** FIX: deshabilitar THIRDPARTY para que no intente buscar fastcdr vía git submodule ***
set(THIRDPARTY          OFF CACHE STRING "" FORCE)
set(THIRDPARTY_fastcdr  OFF CACHE STRING "" FORCE)

# =======================
# Core: FastDDS
# =======================
message(STATUS "[DDS] Fetching fastdds library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.git")
    message(STATUS "Library 'fastdds' found locally at: '${EXTERNAL_LIB_PATH}/fastdds_src'")
    set(FETCHCONTENT_SOURCE_DIR_FASTDDS "${EXTERNAL_LIB_PATH}/fastdds_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    fastdds
    GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS"
    GIT_TAG        v3.2.1
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastdds_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastdds)