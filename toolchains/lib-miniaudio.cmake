# -------------------------------
# Librería de Miniaudio (Audio)
# -------------------------------

message(STATUS "Fetching Miniaudio library...")
set(MA_ENABLE_VORBIS  OFF CACHE BOOL "" FORCE)  # No construir soporte para Vorbis
set(MA_ENABLE_OPUS    OFF CACHE BOOL "" FORCE)  # No construir soporte para Opus
set(MA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)  # No construir ejemplos

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/miniaudio_src/.github")
  message(STATUS "Using local miniaudio source")
  set(FETCHCONTENT_SOURCE_DIR_MINIAUDIO
      "${EXTERNAL_LIB_PATH}/miniaudio_src"
      CACHE PATH "" FORCE)
endif()

# declara el recurso externo que CMake descargará
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.23          # o la versión que necesites
  GIT_SHALLOW    TRUE            # habilita --depth 1
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/miniaudio_src"
  EXCLUDE_FROM_ALL TRUE
)
# Hace disponible el recurso
FetchContent_MakeAvailable(miniaudio)

# Crear una librería estática con Miniaudio
add_library(miniaudio_lib STATIC
  ${miniaudio_SOURCE_DIR}/miniaudio.c
)
target_include_directories(miniaudio_lib PUBLIC ${miniaudio_SOURCE_DIR})
target_link_libraries(miniaudio_lib PUBLIC
  $<$<PLATFORM_ID:Windows>:               # Windows
    winmm                                 # Bibliotecas de Multimedia de Windows
  >

  $<$<PLATFORM_ID:Linux>:                 # Linux
    pthread                               # Hilos POSIX
    dl                                    # Carga dinámica de bibliotecas
    m                                     # Matemáticas (libm)
    asound                                # ALSA (Audio Sound)
  >

  $<$<PLATFORM_ID:APPLE>:                 # macOS/iOS
    "-framework CoreFoundation"           # Marco de fundamentos de Apple
    "-framework CoreAudio"                # Marco de audio de bajo nivel
    "-framework AudioUnit"                # Marco de unidades de audio
  >
)