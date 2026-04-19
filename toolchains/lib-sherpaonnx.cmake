include(FetchContent)
message(STATUS "Fetching sherpa onnx tts...")
cmake_policy(SET CMP0135 NEW) 

set(SHERPA_VERSION "1.12.34")

option(USE_SHERPA_WIN_GPU "Compilar con soporte Windows GPU" ON)      # Activar/desactivar librería compatible con GPU (Windows)

# Configurar URLs según plataforma
if(WIN32)

    # Directorio de descarga y libreria objetivo
    set(SHERPA_INSTALL_PATH "${EXTERNAL_LIB_PATH}/sherpa_win_bin")
    file(MAKE_DIRECTORY "${SHERPA_INSTALL_PATH}")

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
    set(SHERPA_CHECK_LIBS 
        sherpa-onnx-c-api.lib
        sherpa-onnx-cxx-api.lib
    )
else()
    set(SHERPA_INSTALL_PATH "${EXTERNAL_LIB_PATH}/sherpa_linux_bin")
    # Nombre de carpeta/zip y paquete a descargar
    set(SHERPA_LINUX_BIN "sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu" )
    set(SHERPA_LINUX_URL "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_LINUX_BIN}.tar.bz2")
    set(SHERPA_CHECK_LIBS 
        libsherpa-onnx-c-api.so
        libsherpa-onnx-cxx-api.so
        libonnxruntime.so
        libonnxruntime_providers_shared.so
        libonnxruntime_providers_cuda.so
        libonnxruntime_providers_tensorrt.so
    )
endif()

# Crear el directorio de descarga
file(MAKE_DIRECTORY "${SHERPA_INSTALL_PATH}")

# Función de descarga
function(download_sherpa SHERPA_BIN URL)

    set(ALL_LIBS_EXIST TRUE)
    foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
        if (NOT EXISTS "${SHERPA_INSTALL_PATH}/${SHERPA_BIN}/lib/${LIB_NAME}")
            set(ALL_LIBS_EXIST FALSE)
            break()
        endif()
    endforeach()

    if (ALL_LIBS_EXIST)
        message(STATUS "Sherpa-ONNX ${SHERPA_BIN} all libs found locally.")
    else()
        message(STATUS "Sherpa-ONNX ${SHERPA_BIN} missing components. Downloading...")
        # Descarga y descomprimir
        set(TEMP_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/${SHERPA_BIN}.tar.bz2")
        file(DOWNLOAD "${URL}" "${TEMP_ARCHIVE}" SHOW_PROGRESS)
        message(STATUS "Extracting ${TEMP_ARCHIVE}...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${TEMP_ARCHIVE}"
            WORKING_DIRECTORY "${SHERPA_INSTALL_PATH}"
            RESULT_VARIABLE tar_status
        )
    endif()

    # Borrar el archivo comprimido
    if (EXISTS ${TEMP_ARCHIVE})
        file(REMOVE "${TEMP_ARCHIVE}")
    endif()

    # Borrar carpeta bin (no se usa)
    set (SHERPA_BIN_DIR "${SHERPA_INSTALL_PATH}/${SHERPA_BIN}/bin")
    message(STATUS "${SHERPA_BIN_DIR}")
    if(EXISTS "${SHERPA_BIN_DIR}")
        message(STATUS "Deleting ${SHERPA_BIN_DIR} (not used)")
        file(REMOVE_RECURSE "${SHERPA_BIN_DIR}")
    endif()
    
endfunction()


# Configurar según plataforma
if(WIN32)
    
    if (USE_SHERPA_WIN_GPU)
        message(STATUS "Selected sherpa library with GPU compatibility")

        # Versión compatible con GPU (DirectML, aunque no lo ponga), hace también fallback a CPU si falla
        set(SHERPA_WIN_BIN_GPU          "sherpa-onnx-v${SHERPA_VERSION}-win-x64-cuda")
        set(SHERPA_WIN_URL_GPU          "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_GPU}.tar.bz2")

        # Descargar y desplegar el paquete
        download_sherpa(${SHERPA_WIN_BIN_GPU}          ${SHERPA_WIN_URL_GPU})
        
        # Crear la librería de interfaz
        add_library(sherpa_lib INTERFACE)

        # Incluir headers
        target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/include")

        # Link de librerías
        foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
            target_link_libraries(sherpa_lib INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/lib/${LIB_NAME}")
        endforeach()
        target_link_libraries(sherpa_lib INTERFACE 
            ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
            winmm       # Bibliotecas de Multimedia de Windows
        )

    else()
        message(STATUS "Selected sherpa library limited to CPU compatibility")

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

        # Aprovechamos y descargamos todas las configuraciones
        download_sherpa(${SHERPA_WIN_BIN_DEBUG}          ${SHERPA_WIN_URL_DEBUG})
        download_sherpa(${SHERPA_WIN_BIN_RELEASE}        ${SHERPA_WIN_URL_RELEASE})
        download_sherpa(${SHERPA_WIN_BIN_MINSIZEREL}     ${SHERPA_WIN_URL_MINSIZEREL})
        download_sherpa(${SHERPA_WIN_BIN_RELWITHDEBINFO} ${SHERPA_WIN_URL_RELWITHDEBINFO})
        
        # Crear la librería de interfaz
        add_library(sherpa_lib INTERFACE)

        # Incluir headers (Todos son iguales, cualquier config vale)
        target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_RELEASE}/include")

        # Link de librerías (se va a elegir la de la configuración correspondiente)
        foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
            target_link_libraries(sherpa_lib INTERFACE 
                "$<$<CONFIG:Debug>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_DEBUG}/lib/${LIB_NAME}>"
                "$<$<CONFIG:Release>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_RELEASE}/lib/${LIB_NAME}>"
                "$<$<CONFIG:MinSizeRel>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_MINSIZEREL}/lib/${LIB_NAME}>"
                "$<$<CONFIG:RelWithDebInfo>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_RELWITHDEBINFO}/lib/${LIB_NAME}>"
            )
        endforeach()
        target_link_libraries(sherpa_lib INTERFACE 
            ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
            winmm       # Bibliotecas de Multimedia de Windows
        )
    endif()

    
else()
    set(SHERPA_INSTALL_PATH "${EXTERNAL_LIB_PATH}/sherpa_linux_bin")
    file(MAKE_DIRECTORY "${SHERPA_INSTALL_PATH}")

    # Nombre de carpeta/zip y paquete a descargar
    set(SHERPA_LINUX_BIN "sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu" )
    set(SHERPA_LINUX_URL "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_LINUX_BIN}.tar.bz2")
    set(SHERPA_CHECK_LIBS libsherpa-onnx-c-api.so)
    set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/lib")

    # Descarga el binario de Linux
    download_sherpa(${SHERPA_LINUX_BIN}          ${SHERPA_LINUX_URL})

    # Crear la librería
    add_library(sherpa_lib INTERFACE)

    # Incluir headers
    target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/include")

    # Link de librerías 
    set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/lib")     
    target_link_libraries(sherpa_lib INTERFACE 
        "${SHERPA_LIB_PATH}/libsherpa-onnx-c-api.so"
        "${SHERPA_LIB_PATH}/libsherpa-onnx-cxx-api.so"
        "${SHERPA_LIB_PATH}/libonnxruntime.so"
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
#   Funciones
# =================================

# función para el cmakelists, para copiar las dlls y assets en la ruta del exe
function(configure_sherpa_assets)

    # Copiar DLLs necesarias
    if(WIN32)

        # Obtener ruta de librería según CPU / GPU
        if (USE_SHERPA_WIN_GPU)
            set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/lib")
        else()
            set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-$<CONFIG>/lib")
        endif()
        
        # Copiar Las dlls al lado del ejecutable
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHERPA_LIB_PATH}/sherpa-onnx-c-api.dll"
            "${SHERPA_LIB_PATH}/sherpa-onnx-cxx-api.dll"
            "${SHERPA_LIB_PATH}/onnxruntime.dll"
            "${SHERPA_LIB_PATH}/onnxruntime_providers_shared.dll"
            "${SHERPA_LIB_PATH}/onnxruntime_providers_cuda.dll"
            "${SHERPA_LIB_PATH}/onnxruntime_providers_tensorrt.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying Sherpa and ONNX Runtime DLLs ($<CONFIG>)..."
        )
    elseif(UNIX)   
        # Copiar Las dlls al lado del ejecutable
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHERPA_LIB_PATH}/libsherpa-onnx-c-api.so"
            "${SHERPA_LIB_PATH}/libsherpa-onnx-cxx-api.so"
            "${SHERPA_LIB_PATH}/libonnxruntime.so"
            "${SHERPA_LIB_PATH}/libonnxruntime_providers_shared.so"
            "${SHERPA_LIB_PATH}/libonnxruntime_providers_cuda.so"
            "${SHERPA_LIB_PATH}/libonnxruntime_providers_tensorrt.so"
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
