

# ------------------------------
# LIBRERÍA TTS CON
# - ONNX: Runtime que ejecuta el modelo de voz
# - 
# Genera la librería imgui_lib
# ------------------------------

# Comportamiento moderno de CMake
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# ----------------------------------------------------------------
# 1. ONNX Runtime (Binarios precompilados - Recomendado)
# Compilar ONNX desde fuentes toma horas, es mejor bajar el binario
# ----------------------------------------------------------------

# Usa el runtime ya descargado en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/onnxruntime_bin/lib")
  message(STATUS "Using local onnxruntime source")
  set(FETCHCONTENT_SOURCE_DIR_ONNXRUNTIME
      "${EXTERNAL_LIB_PATH}/onnxruntime_bin"
      CACHE PATH "" FORCE)
endif()


set(ONNX_VERSION 1.24.2)
if(WIN32)
    set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-x64-${ONNX_VERSION}.zip")
    set(ONNX_LIB_NAME "onnxruntime.lib")
    set(ONNX_BINARY "onnxruntime.dll")
elseif(LINUX)
    set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-linux-x64-${ONNX_VERSION}.tgz")
    set(ONNX_LIB_NAME "libonnxruntime.so")
    set(ONNX_BINARY "libonnxruntime.so")
endif()

FetchContent_Declare(
    onnxruntime
    URL ${ONNX_URL}
    SOURCE_DIR "${EXTERNAL_LIB_PATH}/onnxruntime_bin"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(onnxruntime)

# ----------------------------------------------------------------
# 2. Piper Phonemize (Dependencia de Piper para fonemas)
# ----------------------------------------------------------------

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/piper_phonemize_src/.github")
  message(STATUS "Using local piper_phonemize source")
  set(FETCHCONTENT_SOURCE_DIR_PIPER_PHONEMIZE
      "${EXTERNAL_LIB_PATH}/piper_phonemize_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    piper_phonemize
    GIT_REPOSITORY https://github.com/rhasspy/piper-phonemize.git
    GIT_TAG 2023.11.14-4
    GIT_SHALLOW    TRUE         # habilita --depth 1
    SOURCE_DIR "${EXTERNAL_LIB_PATH}/piper_phonemize_src"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Forzamos a que no compile sus tests ni programas de ejemplo
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(piper_phonemize)

# ----------------------------------------------------------------
# 3. Piper Core (El motor de voz)
# ----------------------------------------------------------------

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/piper_src/.github")
  message(STATUS "Using local piper source")
  set(FETCHCONTENT_SOURCE_DIR_PIPER
      "${EXTERNAL_LIB_PATH}/piper_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    piper
    GIT_REPOSITORY https://github.com/rhasspy/piper.git
    GIT_TAG master
    SOURCE_DIR "${EXTERNAL_LIB_PATH}/piper_src"
    SOURCE_SUBDIR src/cpp # El código C++ está en esta subcarpeta
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(piper)

# ----------------------------------------------------------------
# 4. Crear la librería piper_lib y configurar dependencias
# ----------------------------------------------------------------
add_library(piper_lib INTERFACE)

# Encontrar la librería de forma dinámica según el compilador
if(WIN32)
    if(MSVC)
        set(ONNX_LINK_TARGET "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib")
    else()
        # Para MinGW/Clang, es más seguro apuntar a la DLL directamente 
        # o dejar que CMake encuentre el archivo de importación
        find_library(ONNX_IMPORT_LIB onnxruntime 
            PATHS "${onnxruntime_SOURCE_DIR}/lib" 
            NO_DEFAULT_PATH)
        set(ONNX_LINK_TARGET ${ONNX_IMPORT_LIB})
    endif()
else()
    set(ONNX_LINK_TARGET "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so")
endif()

target_include_directories(piper_lib INTERFACE 
    "${EXTERNAL_LIB_PATH}/piper_src/src/cpp"
    "${EXTERNAL_LIB_PATH}/piper_phonemize_src/src"
    "${onnxruntime_SOURCE_DIR}/include"
)

target_link_libraries(piper_lib INTERFACE 
    piper_phonemize
    ${ONNX_LINK_TARGET}
)

target_compile_options(asio_lib INTERFACE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
      -Wno-shadow
      -Wno-unused-parameter
      -Wno-pointer-sign
    >
)
