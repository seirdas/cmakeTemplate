# -------------------------------
# Librería de Fast-DDS con dependencias
# genera la librería fastdds_lib
# -------------------------------

# Opciones de compilación de Fast-DDS
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
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

# --------------------------
function(fetch_lib LIB URL TAG)
    message(STATUS "Fetching ${LIB} library...")
    string(TOLOWER ${LIB} LIB_LOW)
    string(TOUPPER ${LIB} LIB_UP)
    set(SRC_PATH "${EXTERNAL_LIB_PATH}/${LIB_LOW}_src")
    # Usa la librería ya descargada en external/ si existe
    if (EXISTS "${EXTERNAL_LIB_PATH}/${LIB_LOW}_src/.git")
    message(STATUS "Using local ${LIB} source")
    set(FETCHCONTENT_SOURCE_DIR_${LIB_UP}
        ${SRC_PATH}
        CACHE PATH "" FORCE)
    endif()
    message(STATUS "URL: " ${URL})
    message(STATUS "TAG: " ${TAG})
    message(STATUS "SRC_PATH: " ${SRC_PATH})
    FetchContent_Declare(
        ${LIB}
        GIT_REPOSITORY ${URL}
        GIT_TAG        ${TAG}  
        SOURCE_DIR     ${SRC_PATH}
        EXCLUDE_FROM_ALL TRUE
    )
    FetchContent_MakeAvailable(${LIB_LOW})
endfunction()
# --------------------------

# 1. Dependencia: Fast-CDR (Obligatoria para Fast-DDS)
fetch_lib("fastcdr"              "https://github.com/eProsima/Fast-CDR"     "v2.3.2")
# 2. Dependencia: foonathan_memory (Gestión de memoria)
fetch_lib("foonathan_memory"     "https://github.com/foonathan/memory"      "v0.7-3")
# 3. FastDDS
fetch_lib("fastdds"              "https://github.com/eProsima/Fast-DDS"     "v3.2.1")


# =========================================================
# NOTA IMPORTANTE: 
# No necesitas add_library(fastdds STATIC IMPORTED) ni target_include_directories.
# El target 'fastdds' ya es exportado automáticamente por eProsima.
# =========================================================