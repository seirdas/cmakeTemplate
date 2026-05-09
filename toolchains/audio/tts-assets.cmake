
# =================================
#   Assets Sherpa (voices)
# =================================

# Directorio de descarga de assets-tts/modelos de voz
set(ASSETS_CMAKE_FOLDER "tts-assets")
set(VOICES_DIR "tts-voices")
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
#   Funcion de copia a la salida
# =================================

function(link_tts_assets)
    set(SRC_DIR "${EXTERNAL_LIB_PATH}/tts-assets/tts-voices")
    file(TO_NATIVE_PATH "${SRC_DIR}" SRC_NATIVE)

    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND @echo ---- Linking tts-assets to output folder...
            COMMAND @echo ${SRC_NATIVE} --- "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # cd para evitar problemas de "/"
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            
            # Borramos el link si ya existía (usando el nombre relativo)
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${VOICES_DIR}"
            
            # Se ejecuta junction (unión)
            COMMAND cmd /c if not exist "${VOICES_DIR}" mklink /J "${VOICES_DIR}" "${SRC_NATIVE}"
        )
    elseif(UNIX)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND ln -sf "${SRC_DIR}" "${VOICES_DIR}"
            VERBATIM
        )
    endif()

    # Translado la ruta de las voces como #define de código c++
    target_compile_definitions(${PROJECT_NAME} PRIVATE VOICES_PATH="${DEFINE_VOICE_DEST_DIR}")

endfunction()