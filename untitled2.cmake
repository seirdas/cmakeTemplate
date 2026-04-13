# =============================================================
# lib-sherpaonnx.cmake
#
# ESTRUCTURA DE ARCHIVOS:
#   Fuentes  →  _external/<nombre>_src/    (compartido por todos los presets)
#   Builds   →  _build/<preset>/_deps/     (por preset, automático)
#
# ESTRATEGIA:
#   Windows  → compilar Sherpa desde fuente con DirectML
#   Linux    → descargar prebuilt GPU (CUDA/CUDNN)
# =============================================================

include(FetchContent)
cmake_policy(SET CMP0135 NEW)

set(SHERPA_TAG "v1.12.38")

# =============================================================
# WINDOWS — Compilación desde fuente con DirectML
# =============================================================
if(WIN32)

    # ─────────────────────────────────────────────────────────
    # Redirigir SOURCE_DIR de las sub-dependencias internas de Sherpa
    # hacia _external, ANTES de que Sherpa haga sus FetchContent_Declare.
    #
    # Regla: FETCHCONTENT_SOURCE_DIR_<NOMBRE_UPPERCASE> sobreescribe
    # el SOURCE_DIR de cualquier FetchContent_Declare con ese nombre.
    # CMake descarga ahí si el dir no existe, o lo reutiliza si existe.
    # Build y subbuild van a _build/<preset>/_deps/<nombre>-build
    # de forma automática sin ninguna configuración extra.
    #
    # Nombres exactos (primer arg de FetchContent_Declare en cmake/ de Sherpa):
    #   cmake/json.cmake                → json
    #   cmake/kaldi-native-fbank.cmake  → kaldi_native_fbank
    #   cmake/kaldi-decoder.cmake       → kaldi_decoder
    #   (dentro de kaldi-decoder)       → kaldifst
    #   (dentro de kaldi-decoder)       → openfst
    #   cmake/simple-sentencepiece.cmake→ simple_sentencepiece
    #   cmake/espeak-ng-for-piper.cmake → espeak_ng
    #   cmake/piper-phonemize.cmake     → piper_phonemize
    #   cmake/onnxruntime.cmake (DML)   → onnxruntime
    # ─────────────────────────────────────────────────────────

    # Aplicar redirecciones de subdependencias
    macro(redirect_sherpa_dep NAME FOLDER_NAME)
        string(TOUPPER ${NAME} UPPER_NAME)
        set(FETCHCONTENT_SOURCE_DIR_${UPPER_NAME}
            ${EXTERNAL_LIB_PATH}/sherpa_${FOLDER_NAME}_src CACHE PATH "" FORCE)
    endmacro()
    redirect_sherpa_dep(json                 json)
    redirect_sherpa_dep(kaldi_native_fbank   kaldi_fbank)
    redirect_sherpa_dep(kaldi_decoder        kaldi_decoder)
    redirect_sherpa_dep(kaldifst             kaldifst)
    redirect_sherpa_dep(openfst              openfst)
    redirect_sherpa_dep(simple_sentencepiece ssentencepiece)
    redirect_sherpa_dep(espeak_ng            espeak)
    redirect_sherpa_dep(piper_phonemize      phonemize)
    redirect_sherpa_dep(onnxruntime          onnxruntime)

    # Opciones de Sherpa (deben ir ANTES de FetchContent_MakeAvailable)
    set(SHERPA_ONNX_ENABLE_TTS             ON  CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_DIRECTML        ON  CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_C_API           ON  CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_USE_STATIC_CRT         OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS                  ON  CACHE BOOL "" FORCE)

    set(SHERPA_ONNX_ENABLE_PYTHON          OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_TESTS           OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_CHECK           OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_PORTAUDIO       OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_JNI             OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_WEBSOCKET       OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_BINARY          OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_WASM            OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_GPU             OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_SPEAKER_DIARIZATION OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_BUILD_C_API_EXAMPLES   OFF CACHE BOOL "" FORCE)
    set(SHERPA_ONNX_ENABLE_SANITIZER       OFF CACHE BOOL "" FORCE)
    set(BUILD_ESPEAK_NG_EXE                OFF CACHE BOOL "" FORCE)
    set(BUILD_ESPEAK_NG_TESTS              OFF CACHE BOOL "" FORCE)
    set(BUILD_PIPER_PHONMIZE_EXE           OFF CACHE BOOL "" FORCE)
    set(BUILD_PIPER_PHONMIZE_TESTS         OFF CACHE BOOL "" FORCE)

    # ─────────────────────────────────────────────────────────
    # Fuente de Sherpa → _external/sherpa_win_src
    # Build → _build/<preset>/_deps/sherpa_onnx-build  (automático)
    # ─────────────────────────────────────────────────────────
    set(_SHERPA_WIN_SRC "${EXTERNAL_LIB_PATH}/sherpa_win_src")

    if(EXISTS "${_SHERPA_WIN_SRC}/CMakeLists.txt")
        message(STATUS "Sherpa-ONNX: fuente local en ${_SHERPA_WIN_SRC}")
    else()
        message(STATUS "Sherpa-ONNX: descargando fuente por primera vez...")
    endif()

    FetchContent_Declare(
        sherpa_onnx
        GIT_REPOSITORY https://github.com/k2-fsa/sherpa-onnx.git
        GIT_TAG        ${SHERPA_TAG}
        GIT_SHALLOW    TRUE
        SOURCE_DIR     "${_SHERPA_WIN_SRC}"
        # No especificamos BINARY_DIR → va a _deps/sherpa_onnx-build
    )
    FetchContent_MakeAvailable(sherpa_onnx)

    # Wrapper INTERFACE
    if(NOT TARGET sherpa_lib)
        add_library(sherpa_lib INTERFACE)
        target_link_libraries(sherpa_lib INTERFACE
            sherpa-onnx-c-api
            ws2_32
            winmm
        )
    endif()


# =============================================================
# LINUX — Prebuilt GPU
# =============================================================
elseif(UNIX)

    set(_SHERPA_LINUX_SRC "${EXTERNAL_LIB_PATH}/sherpa_linux_src/linux")

    if(EXISTS "${_SHERPA_LINUX_SRC}/lib/libsherpa-onnx-c-api.so")
        message(STATUS "Sherpa-ONNX Linux GPU: prebuilt local en ${_SHERPA_LINUX_SRC}")
    else()
        message(STATUS "Sherpa-ONNX Linux GPU: descargando prebuilt...")
    endif()

    FetchContent_Declare(
        sherpa_pkg_linux
        URL        "https://github.com/k2-fsa/sherpa-onnx/releases/download/${SHERPA_TAG}/sherpa-onnx-${SHERPA_TAG}-linux-x64-gpu.tar.bz2"
        SOURCE_DIR "${_SHERPA_LINUX_SRC}"
        EXCLUDE_FROM_ALL TRUE
    )
    FetchContent_MakeAvailable(sherpa_pkg_linux)

    if(NOT TARGET sherpa_lib)
        add_library(sherpa_lib INTERFACE)
        target_include_directories(sherpa_lib INTERFACE
            "${_SHERPA_LINUX_SRC}/include")
        target_link_libraries(sherpa_lib INTERFACE
            "${_SHERPA_LINUX_SRC}/lib/libsherpa-onnx-c-api.so"
            "${_SHERPA_LINUX_SRC}/lib/libonnxruntime.so"
            pthread dl)
        target_compile_options(sherpa_lib INTERFACE
            -Wno-shadow -Wno-unused-parameter -Wno-sign-compare
            -Wno-unused-variable -Wunused-but-set-variable)
    endif()

endif()

# =============================================================
# Assets TTS (voces) — igual para ambas plataformas
# =============================================================

set(ASSETS_CMAKE_FOLDER "tts-assets")
set(VOICES_DIR          "tts-voices")
set(DOWNLOAD_TTS_ASSETS_DIR "${EXTERNAL_LIB_PATH}/${ASSETS_CMAKE_FOLDER}/${VOICES_DIR}")
file(TO_CMAKE_PATH "${VOICES_DIR}" DEFINE_VOICE_DEST_DIR)

function(download_voice NAME URL)
    set(MODEL_DIR "${DOWNLOAD_TTS_ASSETS_DIR}/${NAME}")
    if(NOT EXISTS "${MODEL_DIR}")
        message(STATUS "Descargando modelo TTS: ${NAME}...")
        FetchContent_Declare(${NAME} URL "${URL}" SOURCE_DIR "${MODEL_DIR}")
        FetchContent_Populate(${NAME})
    else()
        message(STATUS "Modelo TTS en caché: ${NAME}")
    endif()
endfunction()

download_voice(vits-piper-en_GB-alan-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-alan-low.tar.bz2")
download_voice(vits-piper-en_GB-southern_english_female-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-southern_english_female-low.tar.bz2")
download_voice(vits-piper-en_US-amy-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-amy-low.tar.bz2")
download_voice(vits-piper-en_US-danny-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-danny-low.tar.bz2")
download_voice(vits-piper-en_US-kathleen-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-kathleen-low.tar.bz2")
download_voice(vits-piper-en_US-lessac-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-lessac-low.tar.bz2")
download_voice(vits-piper-en_US-ryan-low
    "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-ryan-low.tar.bz2")


# =============================================================
# configure_sherpa_assets()
# Llama a esta función en tu CMakeLists DESPUÉS de add_executable
# =============================================================
function(configure_sherpa_assets)

    if(WIN32)
        # onnxruntime directml prebuilt está en sherpa_onnxruntime_src
        set(_ORT "${EXTERNAL_LIB_PATH}/sherpa_onnxruntime_src/lib")

        # sherpa-onnx-c-api.dll la genera CMake → ya está en TARGET_FILE_DIR
        # Solo copiamos las DLLs del prebuilt de onnxruntime
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_ORT}/onnxruntime.dll"
                "${_ORT}/onnxruntime_providers_shared.dll"
                "${_ORT}/DirectML.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copiando onnxruntime + DirectML DLLs...")

    elseif(UNIX)
        set(_LIB "${EXTERNAL_LIB_PATH}/sherpa_linux_src/linux/lib")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_LIB}/libsherpa-onnx-c-api.so"
                "${_LIB}/libonnxruntime.so"
                "${_LIB}/libonnxruntime_providers_shared.so"
                "${_LIB}/libonnxruntime_providers_cuda.so"
                "${_LIB}/libonnxruntime_providers_tensorrt.so"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copiando .so de Sherpa + ONNX Runtime...")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EXTERNAL_LIB_PATH}/tts-assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copiando assets TTS...")

    if(UNIX AND NOT SHERPA_RPATH_CONFIGURED)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH            "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE)
        set(SHERPA_RPATH_CONFIGURED ON CACHE INTERNAL "")
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VOICES_PATH="${DEFINE_VOICE_DEST_DIR}")

endfunction()