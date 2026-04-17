include(FetchContent)
message(STATUS "Fetching sherpa onnx tts...")
cmake_policy(SET CMP0135 NEW) 

set(SHERPA_VERSION "1.12.34")

# - Para Windows ofrecen binarios de todo tipo MENOS la opción
#     compatible con GPU, así que solo procesa con CPU.
#     La única opción es con CUDA (solo NVIDIA, necesita 
#     instalación) o compilar todo el código fuente de 
#     sherpa para generar la versión compatible con
#     DirectML GPU (universal).
# - Para Linux sí hay binario GPU
# (Pendiente de probar binario GPU Windows:
#  https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.12.34/sherpa-onnx-v1.12.34-win-x64-cuda.tar.bz2)

# Configurar URLs según plataforma
if(WIN32)
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_win_bin")
    # Nombre de carpeta/zip (aparentemente es igual que el paquete)
    set(SHERPA_WIN_BIN_DEBUG            "sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Debug" )
    set(SHERPA_WIN_BIN_RELEASE          "sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Release" )
    set(SHERPA_WIN_BIN_MINSIZEREL       "sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-MinSizeRel" )
    set(SHERPA_WIN_BIN_RELWITHDEBINFO   "sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-RelWithDebInfo" )
    # Urls
    set(SHERPA_WIN_URL_DEBUG          "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_DEBUG}.tar.bz2")
    set(SHERPA_WIN_URL_RELEASE        "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_RELEASE}.tar.bz2")
    set(SHERPA_WIN_URL_MINSIZEREL     "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_MINSIZEREL}.tar.bz2")
    set(SHERPA_WIN_URL_RELWITHDEBINFO "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_RELWITHDEBINFO}.tar.bz2")
    set(SHERPA_CHECK_LIB sherpa-onnx-c-api.lib)
else()
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_linux_bin")
    # Nombre de carpeta/zip y paquete a descargar
    set(SHERPA_LINUX_BIN "sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu" )
    set(SHERPA_LINUX_URL "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_LINUX_BIN}.tar.bz2")
    set(SHERPA_CHECK_LIB libsherpa-onnx-c-api.so)
endif()

# Crear el directorio de descarga
file(MAKE_DIRECTORY "${SHERPA_INSTALL_DIR}")

# Función de descarga
function(download_sherpa SHERPA_BIN URL)
    if (EXISTS "${SHERPA_INSTALL_DIR}/${SHERPA_BIN}/lib/${SHERPA_CHECK_LIB}")
        message(STATUS "Sherpa-ONNX ${SHERPA_BIN} found locally.")
    else()
        message(STATUS "Sherpa-ONNX ${SHERPA_BIN} not found. Downloading...")
        # Descarga y descomprimir
        set(TEMP_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/sherpa_${SHERPA_BIN}.tar.bz2")
        file(DOWNLOAD "${URL}" "${TEMP_ARCHIVE}" SHOW_PROGRESS)
        message(STATUS "Extracting ${TEMP_ARCHIVE}...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${TEMP_ARCHIVE}"
            WORKING_DIRECTORY "${SHERPA_INSTALL_DIR}"
            RESULT_VARIABLE tar_status
        )
    endif()

    # Borrar el archivo comprimido
    if (EXISTS ${TEMP_ARCHIVE})
        file(REMOVE "${TEMP_ARCHIVE}")
    endif()

    # Borrar carpeta bin (no se usa)
    set (SHERPA_BIN_DIR "${SHERPA_INSTALL_DIR}/${SHERPA_BIN}/bin")
    message(STATUS "${SHERPA_BIN_DIR}")
    if(EXISTS "${SHERPA_BIN_DIR}")
        message(STATUS "Deleting ${SHERPA_BIN_DIR} (not used)")
        file(REMOVE_RECURSE "${SHERPA_BIN_DIR}")
    endif()
    
endfunction()

if (WIN32)
    # Aprovechamos y descargamos todas las configuraciones
    download_sherpa(${SHERPA_WIN_BIN_DEBUG}          ${SHERPA_WIN_URL_DEBUG})
    download_sherpa(${SHERPA_WIN_BIN_RELEASE}        ${SHERPA_WIN_URL_RELEASE})
    download_sherpa(${SHERPA_WIN_BIN_MINSIZEREL}     ${SHERPA_WIN_URL_MINSIZEREL})
    download_sherpa(${SHERPA_WIN_BIN_RELWITHDEBINFO} ${SHERPA_WIN_URL_RELWITHDEBINFO})

    # Crear la librería de interfaz
    add_library(sherpa_lib INTERFACE)

    # Incluir headers
    target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_DIR}/${SHERPA_WIN_BIN_RELEASE}/include")
    
    # Link de librerías (se va a elegir la de la configuración correspondiente)
    target_link_libraries(sherpa_lib INTERFACE 
        "$<$<CONFIG:Debug>:${SHERPA_INSTALL_DIR}/${SHERPA_WIN_BIN_DEBUG}/lib/${SHERPA_CHECK_LIB}>"
        "$<$<CONFIG:Release>:${SHERPA_INSTALL_DIR}/${SHERPA_WIN_BIN_RELEASE}/lib/${SHERPA_CHECK_LIB}>"
        "$<$<CONFIG:MinSizeRel>:${SHERPA_INSTALL_DIR}/${SHERPA_WIN_BIN_MINSIZEREL}/lib/${SHERPA_CHECK_LIB}>"
        "$<$<CONFIG:RelWithDebInfo>:${SHERPA_INSTALL_DIR}/${SHERPA_WIN_BIN_RELWITHDEBINFO}/lib/${SHERPA_CHECK_LIB}>"
        ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
        winmm       # Bibliotecas de Multimedia de Windows
    )

elseif(UNIX)
    # Descarga el binario de Linux
    download_sherpa(${SHERPA_LINUX_BIN}          ${SHERPA_LINUX_URL})

    # Crear la librería
    add_library(sherpa_lib INTERFACE)

    # Incluir headers
    target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_DIR}/${SHERPA_LINUX_BIN}/include")

    # Link de librerías 
    set(SHERPA_LIB_DIR "${SHERPA_INSTALL_DIR}/${SHERPA_LINUX_BIN}/lib")     
    target_link_libraries(sherpa_lib INTERFACE 
        "${SHERPA_LIB_DIR}/libsherpa-onnx-c-api.so"
        "${SHERPA_LIB_DIR}/libonnxruntime.so"
        pthread
        dl
    )
endif()

# Omitir warnings de la propia librería
target_compile_options(sherpa_lib INTERFACE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
      -Wno-shadow
      -Wno-unused-parameter
      -Wno-sign-compare
      -Wno-unused-variable
      -Wunused-but-set-variable
    >
)

# =================================
#   Assets Sherpa (voices)
# =================================

# Directorio de descarga de assets-tts/modelos de voz
set(ASSETS_CMAKE_FOLDER "tts-assets")
set(VOICES_DIR tts-voices)
set(DOWNLOAD_TTS_ASSETS_DIR "${EXTERNAL_LIB_PATH}/${ASSETS_CMAKE_FOLDER}/${VOICES_DIR}")

# Crear carpeta (si no existe)
file(MAKE_DIRECTORY "${DOWNLOAD_TTS_ASSETS_DIR}")
file(REAL_PATH "${DOWNLOAD_TTS_ASSETS_DIR}" DOWNLOAD_TTS_ASSETS_DIR)

# Ruta como define para usar en código (ver en la función de abajo)
file(TO_CMAKE_PATH "${VOICES_DIR}" DEFINE_VOICE_DEST_DIR)

# Función para descargar las voces
function(download_voice NAME URL)
    # Destino
    set(MODEL_DIR "${DOWNLOAD_TTS_ASSETS_DIR}/${NAME}")
    file(TO_CMAKE_PATH "${DOWNLOAD_TTS_ASSETS_DIR}/${NAME}" MODEL_DIR)
    
    # Archivo temporal descargado
    get_filename_component(FILENAME "${URL}" NAME)
    set(ARCHIVE_PATH "${DOWNLOAD_TTS_ASSETS_DIR}/${FILENAME}")

    # Si ya existe el modelo, no hacer nada
    if(EXISTS "${MODEL_DIR}")
        message(STATUS "Using local TTS Model: ${NAME}...")

        # Borrar archivo comprimido si existe
        if(EXISTS "${ARCHIVE_PATH}")
            file(REMOVE "${ARCHIVE_PATH}")
        endif()

        return()
    endif()

    message(STATUS "Downloading TTS Model: ${NAME}...")

    # Asegurar carpeta base
    file(MAKE_DIRECTORY "${DOWNLOAD_TTS_ASSETS_DIR}")

    # Descargar archivo
    file(DOWNLOAD
        "${URL}"
        "${ARCHIVE_PATH}"
        SHOW_PROGRESS
    )
    # Extraer archivo (tar.bz2 compatible)
    message(STATUS "Extracting ${NAME}...")
    file(MAKE_DIRECTORY "${MODEL_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xvf "${ARCHIVE_PATH}"
        WORKING_DIRECTORY "${DOWNLOAD_TTS_ASSETS_DIR}"
        RESULT_VARIABLE TAR_RESULT
    )
    if(NOT TAR_RESULT EQUAL 0)
        message(FATAL_ERROR "Extraction failed for ${NAME}")
    endif()

    # Borrar archivo comprimido
    if(EXISTS "${ARCHIVE_PATH}")
        file(REMOVE "${ARCHIVE_PATH}")
    endif()

    message(STATUS "TTS Model ${NAME} extracted in ${ARCHIVE_PATH}")
endfunction()

# Añadir aquí las descarga de voces para Sherpa:
download_voice(
    vits-piper-en_GB-alan-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-alan-low.tar.bz2
)
download_voice(
    vits-piper-en_GB-southern_english_female-low 
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-southern_english_female-low.tar.bz2
    )
download_voice(
    vits-piper-en_US-amy-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-amy-low.tar.bz2
)
download_voice(
    vits-piper-en_US-danny-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-danny-low.tar.bz2
)
download_voice(
    vits-piper-en_US-kathleen-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-kathleen-low.tar.bz2
)
download_voice(
    vits-piper-en_US-lessac-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-lessac-low.tar.bz2
)
download_voice(
    vits-piper-en_US-ryan-low
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-ryan-low.tar.bz2
)
download_voice(
    vits-piper-en_US-sam-medium
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_US-sam-medium.tar.bz2
)
download_voice(
    vits-piper-en_GB-miro-high
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-miro-high.tar.bz2
)
download_voice(
    vits-piper-en_GB-alan-medium
    https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-alan-medium.tar.bz2
)


# =================================
#   Funciones
# =================================

# función para el cmakelists, para copiar las dlls y assets en la ruta del exe
function(configure_sherpa_assets)

    # Copiar DLLs necesarias
    if(WIN32)
        # Usamos un generador de expresiones anidado para construir el nombre de la carpeta larga
        set(SHERPA_LIB_DIR "sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-$<CONFIG>/lib")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHERPA_INSTALL_DIR}/${SHERPA_LIB_DIR}/sherpa-onnx-c-api.dll"
            "${SHERPA_INSTALL_DIR}/${SHERPA_LIB_DIR}/onnxruntime.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying Sherpa and ONNX Runtime DLLs ($<CONFIG>)..."
        )
    elseif(UNIX)   
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHERPA_LIB_DIR}/libsherpa-onnx-c-api.so"
            "${SHERPA_LIB_DIR}/libonnxruntime.so"
            "${SHERPA_LIB_DIR}/libonnxruntime_providers_shared.so"
            "${SHERPA_LIB_DIR}/libonnxruntime_providers_cuda.so"
            "${SHERPA_LIB_DIR}/libonnxruntime_providers_tensorrt.so"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying Sherpa and ONNX Runtime Shared Libs (GPU support)..."
        )
    endif()

    # Copiar assets de voces
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIB_PATH}/tts-assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying TTS voice assets..."
    )

    # Los siguientes target_properties hay que hacerlos cuando el proyecto esté creado:

    # Linux: Configurar RPATH para que el ejecutable encuentre las .so al lado del binario
    if (UNIX AND NOT SHERPA_RPATH_CONFIGURED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
        set(SHERPA_RPATH_CONFIGURED ON CACHE INTERNAL "RPATH has been set for Sherpa")
    endif()

    # Translado la ruta de las voces como #define de código c++
    target_compile_definitions(${PROJECT_NAME} PRIVATE VOICES_PATH="${DEFINE_VOICE_DEST_DIR}")

endfunction()