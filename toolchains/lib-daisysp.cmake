# -------------------------------
# Librería DaisySP (DSP)
# -------------------------------

message(STATUS "Fetching DaisySP library...")
set(BUILD_DAISYSP_EXAMPLES  OFF CACHE BOOL "" FORCE)    # No construir ejemplos

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/daisysp_src/.github")
  message(STATUS "Using local daisysp source")
  set(FETCHCONTENT_SOURCE_DIR_DAISYSP
      "${EXTERNAL_LIB_PATH}/daisysp_src"
      CACHE PATH "" FORCE)
endif()

# declara el recurso externo que CMake descargará
FetchContent_Declare(
  daisysp
  GIT_REPOSITORY https://github.com/electro-smith/DaisySP.git
  GIT_TAG        V1.0.0
  GIT_SHALLOW    TRUE
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/daisysp_src"
  EXCLUDE_FROM_ALL TRUE
)
# Hace disponible el recurso
FetchContent_MakeAvailable(daisysp)

file(GLOB_RECURSE DAISYSP_SOURCES
  "${daisysp_SOURCE_DIR}/DaisySP-LGPL/Source/*.cpp"
)

# Crear una librería estática con DaisySP
add_library(daisysp_lib STATIC ${DAISYSP_SOURCES})
target_compile_definitions(daisysp_lib PUBLIC
  DAISYSP_DESKTOP=1   # Supone entorno de escritorio, no sistema embebido
  _USE_MATH_DEFINES
)
target_include_directories(daisysp_lib PUBLIC
  "${daisysp_SOURCE_DIR}/Include"
  "${daisysp_SOURCE_DIR}/Source"
)
if (MSVC)
  target_compile_options(daisysp_lib PRIVATE /fp:fast)
else()
  target_compile_options(daisysp_lib PRIVATE -ffast-math)
endif()
target_link_libraries(daisysp_lib PRIVATE
    $<$<PLATFORM_ID:Linux>:asound>
)