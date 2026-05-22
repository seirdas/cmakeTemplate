# -------------------------------
# Librería de tinyxml2
# genera la librería tinyxml2_lib
# -------------------------------

include(FetchContent)

message(STATUS "[tinyxml2] Fetching tinyxml2 library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/tinyxml2_src/.git")
  message(STATUS "[tinyxml2] Library 'tinyxml2' found locally at: '${EXTERNAL_LIB_PATH}/tinyxml2_src'")
  set(FETCHCONTENT_SOURCE_DIR_TINYXML2
      "${EXTERNAL_LIB_PATH}/tinyxml2_src"
      CACHE PATH "" FORCE)
endif()
fetchcontent_declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        11.0.0      # Versión estable más reciente
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/tinyxml2_src"
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL TRUE
)
fetchcontent_makeavailable(tinyxml2)

# Crear la librería estática
add_library(tinyxml2_lib STATIC "${tinyxml2_SOURCE_DIR}/tinyxml2.cpp")
target_include_directories(tinyxml2_lib PUBLIC "${tinyxml2_SOURCE_DIR}")
target_compile_definitions(tinyxml2_lib PUBLIC TINYXML2_DEBUG)

# Omitir warnings de la propia librería
if (MSVC)
    # --- Configuración para MSVC (Visual Studio) ---
    target_compile_options(tinyxml2_lib PRIVATE
        /W0            # Nivel de advertencia 0 (silencio total)
        /wd4244        # double a float
        /wd4305        # truncamiento de constantes
        /wd4267        # size_t a int
        /external:W0   # (CMake 3.22+) Silencia cabeceras externas
    )
else()
    # --- Configuración para GCC / Clang / MinGW ---
    target_compile_options(tinyxml2_lib PRIVATE
        -w             # Suprime todos los warnings
        -Wno-conversion
        -Wno-sign-compare
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-shadow
    )
endif()