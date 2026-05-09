# -------------------------------
# Librería de stb con dependencias
# genera la librería stb-lib
# -------------------------------

include(FetchContent)

message(STATUS "[stb] Fetching stb library...")

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
    GIT_TAG          master      # No hay un tag específico
    GIT_SHALLOW      TRUE        # habilita --depth 1
    SOURCE_DIR       "${EXTERNAL_LIB_PATH}/stb_src"
    EXCLUDE_FROM_ALL TRUE
)
# Hacerla disponible
FetchContent_MakeAvailable(stb)

# Crear librería INTERFACE
add_library(stb_lib INTERFACE)

target_include_directories(stb_lib INTERFACE
    ${stb_SOURCE_DIR}
)
