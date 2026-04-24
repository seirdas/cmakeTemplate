# -------------------------------
# Librería de Fast-DDS con dependencias
# genera la librería fastdds_lib
# -------------------------------

set(URL     "https://github.com/eProsima/Fast-DDS.git")
set(TAG     "v3.2.1")

# Opciones de compilación de Fast-DDS
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)


# =======================
# Lógica de descarga
# =======================

# --------------------------
# 1. Dependencia: Fast-CDR (Obligatoria para Fast-DDS)
# --------------------------
set(LIB_NAME "fastcdr")
string(TOLOWER ${LIB_NAME} LIB_NAME_LOWER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
set(GIT_TAG "v2.2.4")   # Versión compatible con Fast-DDS v3.x
set(GIT_REPOSITORY "https://github.com/eProsima/Fast-CDR.git")
message(STATUS "Fetching ${LIB_NAME} library...")
# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src/.github")
  message(STATUS "Using local ${LIB_NAME} source")
  set(FETCHCONTENT_SOURCE_DIR_${LIB_NAME_UPPER}
      "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    ${LIB_NAME}
    GIT_REPOSITORY ${GIT_REPOSITORY}
    GIT_TAG        ${GIT_TAG}  
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src"
    EXCLUDE_FROM_ALL TRUE
)

# --------------------------
# 2. Dependencia: foonathan_memory (Gestión de memoria)
# --------------------------
set(LIB_NAME "foonathan_memory")
string(TOLOWER ${LIB_NAME} LIB_NAME_LOWER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
message(STATUS "Fetching ${LIB_NAME} library...")
# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src/.github")
  message(STATUS "Using local ${LIB_NAME} source")
  set(FETCHCONTENT_SOURCE_DIR_${LIB_NAME_UPPER}
      "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY https://github.com/foonathan/memory.git
    GIT_TAG        v0.7-3
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/foonathan_memory_src"
    EXCLUDE_FROM_ALL TRUE
)

# --------------------------
# 3. FastDDS
# --------------------------
set(LIB_NAME "foonathan_memory")
string(TOLOWER ${LIB_NAME} LIB_NAME_LOWER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
message(STATUS "Fetching ${LIB_NAME} library...")
# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src/.github")
  message(STATUS "Using local ${LIB_NAME} source")
  set(FETCHCONTENT_SOURCE_DIR_${LIB_NAME_UPPER}
      "${EXTERNAL_LIB_PATH}/${LIB_NAME_LOWER}_src"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    ${LIB_NAME}
    GIT_REPOSITORY   ${URL}
    GIT_TAG          ${TAG}  
    GIT_SHALLOW      TRUE    # habilita --depth 1
    SOURCE_DIR       "${EXTERNAL_LIB_PATH}/${LIB_NAME}_src"
    EXCLUDE_FROM_ALL TRUE
)

# Hacerla disponible
FetchContent_MakeAvailable(${LIB_NAME_LOWER})

# Crear una librería estática
add_library(${LIB_NAME_LOWER} STATIC IMPORTED)


# =======================
# Propiedades específicas de librería
# =======================

set_target_properties(${LIB_NAME_LOWER} PROPERTIES
    IMPORTED_LOCATION "${fastdds_SOURCE_DIR}/lib/libfastdds.a"  
)

# Incluir directorios de Fast-DDS
target_include_directories(${LIB_NAME_LOWER} INTERFACE 
    "${fastdds_SOURCE_DIR}/src/cpp" 
    "${fastdds_SOURCE_DIR}/include"
)