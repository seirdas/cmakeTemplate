
# ------------------------------
# LIBRERÍA CON JUCE AUDIO
# ------------------------------

include(FetchContent)

message(STATUS "Fetching JUCE library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/juce_src/.github")
  message(STATUS "Using local JUCE source")
  set(FETCHCONTENT_SOURCE_DIR_JUCE
      "${EXTERNAL_LIB_PATH}/juce_src"
      CACHE PATH "" FORCE)
endif()

# declara el recurso externo que CMake descargará y hará disponible como una sub‑carpeta del proyecto
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE              # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/juce_src"
    EXCLUDE_FROM_ALL TRUE
)

# Opciones para que JUCE no compile todo el framework (ejemplos, etc.)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_COPY_PLUGIN_AFTER_BUILD OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(juce)