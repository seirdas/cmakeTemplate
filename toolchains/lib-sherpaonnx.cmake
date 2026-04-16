include(FetchContent)
message(STATUS "Fetching sherpa onnx tts...")

cmake_policy(SET CMP0135 NEW) 

set(SHERPA_VERSION "1.12.34")

# Configurar URLs según plataforma
if(WIN32)
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_win_src")
    set(URL_DEBUG          "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Debug.tar.bz2")
    set(URL_RELEASE        "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Release.tar.bz2")
    set(URL_MINSIZEREL     "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-MinSizeRel.tar.bz2")
    set(URL_RELWITHDEBINFO "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-RelWithDebInfo.tar.bz2")
    set(SHERPA_CHECK_LIB sherpa-onnx-c-api.lib)
else()
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_linux_src")
    set(URL_LINUX "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu.tar.bz2")
    set(SHERPA_CHECK_LIB libsherpa-onnx-c-api.so)
endif()

# Función de descarga
function(download_sherpa CONFIG URL)
    string(TOLOWER ${CONFIG} CONFIG_LOWER)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(DEST_DIR "${SHERPA_INSTALL_DIR}/${CONFIG_LOWER}")

    if (EXISTS "${DEST_DIR}/lib/${SHERPA_CHECK_LIB}")
        message(STATUS "Sherpa-ONNX ${CONFIG} found locally at ${DEST_DIR}")
        set(FETCHCONTENT_SOURCE_DIR_SHERPA_PKG_${CONFIG_UPPER} "${DEST_DIR}" CACHE PATH "" FORCE)
    else()
        message(STATUS "Sherpa-ONNX ${CONFIG} not found. Downloading...")
    endif()

    FetchContent_Declare(
        sherpa_pkg_${CONFIG_LOWER}
        URL ${URL}
        SOURCE_DIR "${DEST_DIR}"
        GIT_SHALLOW    TRUE        # habilita --depth 1
        EXCLUDE_FROM_ALL TRUE
    )
    FetchContent_MakeAvailable(sherpa_pkg_${CONFIG_LOWER})
endfunction()

if (WIN32)
    download_sherpa(Debug          ${URL_DEBUG})
    download_sherpa(Release        ${URL_RELEASE})
    download_sherpa(MinSizeRel     ${URL_MINSIZEREL})
    download_sherpa(RelWithDebInfo ${URL_RELWITHDEBINFO})
else()
    download_sherpa(linux          ${URL_LINUX})
endif()

# Crear la librería de interfaz
add_library(sherpa_lib INTERFACE)

# Link de la librería (.lib / .so) por configuración
if(WIN32)
    # Headers, todas son iguales (por ejemplo release)
    target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_DIR}/release/include")
    
    # Librerías
    foreach(CFG Debug Release Minsizerel Relwithdebinfo)
        string(TOLOWER ${CFG} L_CONFIG)
        set(LIB_PATH "${SHERPA_INSTALL_DIR}/${L_CONFIG}/lib")
        
        target_link_libraries(sherpa_lib INTERFACE 
            "$<$<CONFIG:${CFG}>:${LIB_PATH}/sherpa-onnx-c-api.lib>"
        )
    endforeach()

    target_link_libraries(sherpa_lib INTERFACE 
        ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
        winmm       # Bibliotecas de Multimedia de Windows
    )

elseif(UNIX)
    # Headers
    target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_DIR}/linux/include")

    # Librerías
    set(LIB_PATH "${SHERPA_INSTALL_DIR}/linux/lib")
    target_link_libraries(sherpa_lib INTERFACE 
        "${LIB_PATH}/libsherpa-onnx-c-api.so"
        "${LIB_PATH}/libonnxruntime.so"
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

# Directorio de assets tts
set(ASSETS_CMAKE_FOLDER "tts-assets")

# Directorio de modelos de voz
set(VOICES_DIR tts-voices)

set(DOWNLOAD_TTS_ASSETS_DIR "${EXTERNAL_LIB_PATH}/${ASSETS_CMAKE_FOLDER}/${VOICES_DIR}")

# Crear primero
file(MAKE_DIRECTORY "${DOWNLOAD_TTS_ASSETS_DIR}")

# Luego resolver
file(REAL_PATH "${DOWNLOAD_TTS_ASSETS_DIR}" DOWNLOAD_TTS_ASSETS_DIR)

# Fix Windows para rutas muy largas (pasar a formato 8.3)
if(WIN32)
    # Obtener ruta absoluta normalizada
    file(REAL_PATH "${DOWNLOAD_TTS_ASSETS_DIR}" ABS_PATH)

    # Obtener 8.3 con comando de Windows
    execute_process(
        COMMAND cmd /c for %I in ("${ABS_PATH}") do @echo %~sI
        OUTPUT_VARIABLE SHORT_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Limpiar posibles retornos raros
    string(REPLACE "\r" "" SHORT_PATH "${SHORT_PATH}")
    string(REPLACE "\n" "" SHORT_PATH "${SHORT_PATH}")

    # Crear path formato 8.3
    file(TO_CMAKE_PATH "${SHORT_PATH}" DOWNLOAD_TTS_ASSETS_DIR)
endif()

message(STATUS "DOWNLOAD_TTS_ASSETS_DIR : " ${DOWNLOAD_TTS_ASSETS_DIR})

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
        STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Download failed for ${NAME}")
    endif()

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
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHERPA_INSTALL_DIR}/$<LOWER_CASE:$<CONFIG>>/lib/sherpa-onnx-c-api.dll"
            "${SHERPA_INSTALL_DIR}/$<LOWER_CASE:$<CONFIG>>/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying Sherpa and ONNX Runtime DLLs ($<CONFIG>)..."
        )
    elseif(UNIX)
        set(LIB_DIR "${SHERPA_INSTALL_DIR}/linux/lib")
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIB_DIR}/libsherpa-onnx-c-api.so"
            "${LIB_DIR}/libonnxruntime.so"
            "${LIB_DIR}/libonnxruntime_providers_shared.so"
            "${LIB_DIR}/libonnxruntime_providers_cuda.so"
            "${LIB_DIR}/libonnxruntime_providers_tensorrt.so"
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