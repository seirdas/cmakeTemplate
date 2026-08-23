# -------------------------------
# Librería de stb con dependencias
# genera la librería stb-lib
# -------------------------------
include(FetchContent)
message(STATUS "[stb] Fetching stb library...")

# Versiones de librerías
set(STB_VERSION         master)       # No hay un tag específico

# Versión para devolver al CMakeLists principal
set(LIB_VERSION ${STB_VERSION})


# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/stb_src/.git")
  message(STATUS "[stb] Library 'stb' found locally at: '${EXTERNAL_LIB_PATH}/stb_src'")
  set(FETCHCONTENT_SOURCE_DIR_STB
      "${EXTERNAL_LIB_PATH}/stb_src"
      CACHE PATH "" FORCE)
endif()

# Declarar dependencia externa
FetchContent_Declare(
    stb
    GIT_REPOSITORY   https://github.com/nothings/stb
    GIT_TAG          ${STB_VERSION}    
    GIT_SHALLOW      TRUE        # habilita --depth 1
    SOURCE_DIR       "${EXTERNAL_LIB_PATH}/stb_src"
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL TRUE
)
# Hacerla disponible
FetchContent_MakeAvailable(stb)

# Crear librería INTERFACE
add_library(stb_lib INTERFACE)

target_include_directories(stb_lib SYSTEM INTERFACE
    ${stb_SOURCE_DIR}
)

# Gestión de licencia
if(EXISTS "${THIRD_PARTY_LICENSES_FILE}")
    set(LIB_LICENSE_PATH "${stb_SOURCE_DIR}/LICENSE")

    if(EXISTS "${LIB_LICENSE_PATH}")
        file(READ "${LIB_LICENSE_PATH}" LIB_LICENSE_TEXT)
        
        file(APPEND "${THIRD_PARTY_LICENSES_FILE}"
            "------------------------------------------------------------------------\n"
            " Library: stb (Version: ${LIB_VERSION})\n"
            "------------------------------------------------------------------------\n\n"
            "${LIB_LICENSE_TEXT}\n\n"
        )
    else()
        message(WARNING "License file not found in: ${LIB_LICENSE_PATH}")
    endif()
endif()
