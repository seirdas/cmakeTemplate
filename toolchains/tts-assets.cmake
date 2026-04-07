# toolchains/tts-assets.cmake

set(TTS_DATA_DIR "${EXTERNAL_LIB_PATH}/tts-assets")

# Crear carpeta de recursos si no existe
if(NOT EXISTS "${TTS_DATA_DIR}")
    file(MAKE_DIRECTORY "${TTS_DATA_DIR}")
endif()

# -------------------------------
# Kokoro TTS ONNX Models (solo se usa uno)
# -------------------------------
set(KOKORO_MODEL_FILE "${TTS_DATA_DIR}/kokoro-v1.0.onnx")
set(MODEL_URL "https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx")
if(NOT EXISTS "${KOKORO_MODEL_FILE}")
    message(STATUS "Downloading: kokoro-v1.0.onnx: (310MB): optimized f32 version from taylorchu/kokoro-onnx...")
    file(MAKE_DIRECTORY "${TTS_DATA_DIR}")
    file(DOWNLOAD "${MODEL_URL}" "${KOKORO_MODEL_FILE}" SHOW_PROGRESS)
endif()

set(KOKORO_MODEL_FILE "${TTS_DATA_DIR}/kokoro-v1.0.fp16.onnx")
set(MODEL_URL "https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.fp16.onnx")
if(NOT EXISTS "${KOKORO_MODEL_FILE}")
    message(STATUS "Downloading: kokoro-v1.0.fp16.onnx: (169MB): optimized f16 version from taylorchu/kokoro-onnx...")
    file(MAKE_DIRECTORY "${TTS_DATA_DIR}")
    file(DOWNLOAD "${MODEL_URL}" "${KOKORO_MODEL_FILE}" SHOW_PROGRESS)
endif()

set(KOKORO_MODEL_FILE "${TTS_DATA_DIR}/kokoro-v1.0.int8.onnx")
set(MODEL_URL "https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.int8.onnx")
if(NOT EXISTS "${KOKORO_MODEL_FILE}")
    message(STATUS "Downloading: kokoro-v1.0.int8.onnx: (88MB): optimized int8 version from taylorchu/kokoro-onnx...")
    file(MAKE_DIRECTORY "${TTS_DATA_DIR}")
    file(DOWNLOAD "${MODEL_URL}" "${KOKORO_MODEL_FILE}" SHOW_PROGRESS)
endif()


# -------------------------------
# Phonemizer ONNX assets
# -------------------------------

set(PHONEMIZER_MODEL_FILE "${TTS_DATA_DIR}/phonemizer.onnx")
set(PHONEMIZER_MODEL_URL
"https://huggingface.co/lookbe/open-phonemizer-onnx/resolve/main/model.onnx")
if(NOT EXISTS "${PHONEMIZER_MODEL_FILE}")
    message(STATUS "Downloading phonemizer ONNX...")
    file(DOWNLOAD "${PHONEMIZER_MODEL_URL}" "${PHONEMIZER_MODEL_FILE}" SHOW_PROGRESS)
endif()


set(PHONEMIZER_DICT_FILE "${TTS_DATA_DIR}/phoneme_dict.json")
set(PHONEMIZER_DICT_URL
"https://huggingface.co/lookbe/open-phonemizer-onnx/resolve/main/phoneme_dict.json")
if(NOT EXISTS "${PHONEMIZER_DICT_FILE}")
    message(STATUS "Downloading phoneme_dict.json...")
    file(DOWNLOAD "${PHONEMIZER_DICT_URL}" "${PHONEMIZER_DICT_FILE}" SHOW_PROGRESS)
endif()


set(PHONEMIZER_TOKENIZER_FILE "${TTS_DATA_DIR}/phonemizer_tokenizer.json")
set(PHONEMIZER_TOKENIZER_URL
"https://huggingface.co/lookbe/open-phonemizer-onnx/resolve/main/tokenizer.json")

if(NOT EXISTS "${PHONEMIZER_TOKENIZER_FILE}")
    message(STATUS "Downloading tokenizer.json...")
    file(DOWNLOAD "${PHONEMIZER_TOKENIZER_URL}" "${PHONEMIZER_TOKENIZER_FILE}" SHOW_PROGRESS)
endif()


# -------------------------------
# Voice dataset
# -------------------------------

set(VOICES_JSON "${TTS_DATA_DIR}/voices.json")
set(VOICES_URL "https://huggingface.co/datasets/ecyht2/kokoro-82M-voices/resolve/main/voices.json")
if(NOT EXISTS "${VOICES_JSON}")
    message(STATUS "Downloading voice database (voices.json)...")
    file(DOWNLOAD "${VOICES_URL}" "${VOICES_JSON}" SHOW_PROGRESS STATUS download_status)
    
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(WARNING "Cannot download voices.json. Check network or download link from tts-assets.cmake")
    endif()
endif()

# -------------------------------POST-BUILD

# Define la función para copiar archivos y directorios 
# SE USA EN EL POST-BUILD
function(copy_tts_assets)
    # Copiar la carpeta de assets de Kokoro al directorio del ejecutable
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TTS_DATA_DIR}/"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/tts-assets"
        COMMENT "Copying TTS assets..."
    )
endfunction()



# >>>>>>>>>>>>>>>>>>>>>>> WIP: SHERPA ONNX <<<<<<<<<<<<<<<<<<<<<<<<<<


# =========================================================
# 1. POLÍTICAS PARA SILENCIAR WARNINGS DE CMAKE
# =========================================================
cmake_policy(SET CMP0169 OLD) # Silencia warnings de FetchContent_Populate
cmake_policy(SET CMP0177 OLD) # Silencia warnings de install() DESTINATION

# =========================================================
# 2. EVITAR DUPLICIDAD DE ASIO
# =========================================================
# Sherpa usa internamente el nombre "asio" (minúsculas). 
# Le decimos que el código fuente ya existe en tu carpeta local.

if (EXISTS "${EXTERNAL_LIB_PATH}/asio_src/.github")
    set(FETCHCONTENT_SOURCE_DIR_ASIO "${EXTERNAL_LIB_PATH}/asio_src" CACHE PATH "" FORCE)
    set(asio_SOURCE_DIR "${EXTERNAL_LIB_PATH}/asio_src" CACHE PATH "" FORCE)
endif()

# ==========================================
# 1. SILENCIAR TESTS Y RUIDO DE EIGEN/SHERPA
# ==========================================
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_LEAVE_TEST_IN_ALL_TARGET OFF CACHE BOOL "" FORCE)

# ==========================================
# 2. EL TRUCO PARA ENGAÑAR A SHERPA-ONNX
# ==========================================
# Sherpa busca estas dos variables exactas. Si se las damos hechas, 
# ignora su script que falla en Clang/MinGW.

# En el bloque donde configuras Sherpa-ONNX:
set(location_onnxruntime_lib "onnxruntime_lib" CACHE STRING "Forced ONNX target" FORCE)
set(location_onnxruntime_header_dir "${ONNX_INSTALL_DIR}/include" CACHE PATH "" FORCE)

set(SHERPA_ONNX_USE_PRE_INSTALLED_ONNXRUNTIME_IF_AVAILABLE ON CACHE BOOL "" FORCE)

# ==========================================
# 3. CONFIGURAR SHERPA-ONNX
# ==========================================
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)



# =========================================================
# REDIRECCIÓN DE DEPENDENCIAS A CARPETAS PERSISTENTES
# =========================================================

set(SHERPA_DEPS 
    "cppjieba" "eigen" "espeak_ng" "hclust_cpp" 
    "kaldifst" "kaldi_decoder" "kaldi_native_fbank" 
    "openfst" "piper_phonemize" "simple-sentencepiece" "websocketpp"
)

foreach(DEP ${SHERPA_DEPS})
    # Normalizar nombre (ej: simple-sentencepiece -> SIMPLE_SENTENCEPIECE)
    string(REPLACE "-" "_" DEP_VAR ${DEP})
    string(TOUPPER ${DEP_VAR} DEP_UPPER)

    # Usa la librería ya descargada en external/ si existe
    if (EXISTS "${EXTERNAL_LIB_PATH}/${DEP}_src/.github")
        message(STATUS "Using local ${DEP} source")
        set(FETCHCONTENT_SOURCE_DIR_${DEP_UPPER}
            "${EXTERNAL_LIB_PATH}/${DEP}_src"
            CACHE PATH "" FORCE)
    endif()

    FetchContent_Declare(
        ${DEP}
        GIT_SHALLOW    TRUE        # habilita --depth 1
        SOURCE_DIR     "${EXTERNAL_LIB_PATH}/${DEP}_src"
        EXCLUDE_FROM_ALL TRUE
    )
endforeach()


# Desactivamos todo lo que no es estrictamente el core o C API
set(SHERPA_ONNX_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_BINARY OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_BUILD_C_API_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_CHECK OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_JNI OFF CACHE BOOL "" FORCE)

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/sherpa_onnx_src/.github")
  message(STATUS "Using local sherpa_onnx_src source")
  set(FETCHCONTENT_SOURCE_DIR_SHERPA_ONNX
      "${EXTERNAL_LIB_PATH}/sherpa_onnx_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    sherpa_onnx
    GIT_REPOSITORY https://github.com/k2-fsa/sherpa-onnx.git
    GIT_TAG v1.12.34
    GIT_SHALLOW TRUE         # habilita --depth 1
    SOURCE_DIR "${EXTERNAL_LIB_PATH}/sherpa_onnx_src"
    EXCLUDE_FROM_ALL TRUE
)

# Forzar el soporte de C++14 para evitar el falso negativo en simple-sentencepiece
set(SBPE_COMPILER_SUPPORTS_CXX14 ON CACHE BOOL "Forced C++14 support" FORCE)

# MakeAvailable configura y añade los directorios de las dependencias al proyecto
FetchContent_MakeAvailable(${SHERPA_DEPS} sherpa_onnx)

# Interfaz limpia para tu proyecto
add_library(sherpa_onnx_lib INTERFACE)
target_link_libraries(sherpa_onnx_lib INTERFACE sherpa-onnx-c-api ${location_onnxruntime_lib})
target_include_directories(sherpa_onnx_lib INTERFACE 
    "${sherpa_onnx_SOURCE_DIR}"
    "${sherpa_onnx_SOURCE_DIR}/sherpa-onnx/c-api"
)