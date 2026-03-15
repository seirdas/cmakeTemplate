# toolchains/lib-kokoro-data.cmake

set(TTS_DATA_DIR "${EXTERNAL_LIB_PATH}/tts-assets")

# Crear carpeta de recursos si no existe
if(NOT EXISTS "${TTS_DATA_DIR}")
    file(MAKE_DIRECTORY "${TTS_DATA_DIR}")
endif()


# -------------------------------
# Kokoro TTS ONNX Model asset
# -------------------------------
set(KOKORO_MODEL_FILE "${TTS_DATA_DIR}/kokoro-v1.0.onnx")
set(MODEL_URL "https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx")
if(NOT EXISTS "${KOKORO_MODEL_FILE}")
    message(STATUS "Descargando: kokoro-v1.0.onnx: (310MB): optimized f32 version from taylorchu/kokoro-onnx...")
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
    message(STATUS "Descargando phonemizer ONNX...")
    file(DOWNLOAD "${PHONEMIZER_MODEL_URL}" "${PHONEMIZER_MODEL_FILE}" SHOW_PROGRESS)
endif()


set(PHONEMIZER_DICT_FILE "${TTS_DATA_DIR}/phoneme_dict.json")
set(PHONEMIZER_DICT_URL
"https://huggingface.co/lookbe/open-phonemizer-onnx/resolve/main/phoneme_dict.json")
if(NOT EXISTS "${PHONEMIZER_DICT_FILE}")
    message(STATUS "Descargando phoneme_dict.json...")
    file(DOWNLOAD "${PHONEMIZER_DICT_URL}" "${PHONEMIZER_DICT_FILE}" SHOW_PROGRESS)
endif()


set(PHONEMIZER_TOKENIZER_FILE "${TTS_DATA_DIR}/phonemizer_tokenizer.json")
set(PHONEMIZER_TOKENIZER_URL
"https://huggingface.co/lookbe/open-phonemizer-onnx/resolve/main/tokenizer.json")

if(NOT EXISTS "${PHONEMIZER_TOKENIZER_FILE}")
    message(STATUS "Descargando tokenizer.json...")
    file(DOWNLOAD "${PHONEMIZER_TOKENIZER_URL}" "${PHONEMIZER_TOKENIZER_FILE}" SHOW_PROGRESS)
endif()


# -------------------------------
# Voice dataset
# -------------------------------

set(VOICES_JSON "${TTS_DATA_DIR}/voices.json")
set(VOICES_URL "https://huggingface.co/datasets/ecyht2/kokoro-82M-voices/resolve/main/voices.json")
if(NOT EXISTS "${VOICES_JSON}")
    message(STATUS "Descargando base de datos de voces (voices.json)...")
    file(DOWNLOAD "${VOICES_URL}" "${VOICES_JSON}" SHOW_PROGRESS STATUS download_status)
    
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(WARNING "No se pudo descargar voices.json automáticamente. Verifica la conexión o el enlace.")
    endif()
endif()

# -------------------------------POST-BUILD

# Define la función para copiar archivos y directorios 
# SE USA EN EL POST-BUILD
function(copy_tts_assets)
    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNX_INSTALL_DIR}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copiando ONNX Runtime DLL desde ${ONNX_INSTALL_DIR}..."
        )
    endif()

    # Copiar la carpeta de assets de Kokoro al directorio del ejecutable
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TTS_DATA_DIR}/"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/tts-assets"
        COMMENT "Copiando assets de Kokoro..."
    )
endfunction()