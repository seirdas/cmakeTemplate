include(FetchContent)
message(STATUS "Fetching sherpa onnx tts...")

cmake_policy(SET CMP0135 NEW) 

set(SHERPA_VERSION "1.12.30")

# Configurar URLs según plataforma
if(WIN32)
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_win_src")
    set(URL_DEBUG          "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Debug.tar.bz2")
    set(URL_RELEASE        "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-Release.tar.bz2")
    set(URL_MINSIZEREL     "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-MinSizeRel.tar.bz2")
    set(URL_RELWITHDEBINFO "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-win-x64-shared-MD-RelWithDebInfo.tar.bz2")
else()
    set(SHERPA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/sherpa_linux_src")
    set(URL_LINUX "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu.tar.bz2")
endif()

# Función de descarga
function(download_sherpa CONFIG URL_STR)
    string(TOLOWER ${CONFIG} CONFIG_LOWER)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(DEST_DIR "${SHERPA_INSTALL_DIR}/${CONFIG_LOWER}")

    if (EXISTS "${DEST_DIR}/lib/sherpa-onnx-c-api.lib")
        message(STATUS "Sherpa-ONNX ${CONFIG} found locally at ${DEST_DIR}")
        set(FETCHCONTENT_SOURCE_DIR_SHERPA_PKG_${CONFIG_UPPER} "${DEST_DIR}" CACHE PATH "" FORCE)
    else()
        message(STATUS "Sherpa-ONNX ${CONFIG} not found. Downloading...")
    endif()

    FetchContent_Declare(
        sherpa_pkg_${CONFIG_LOWER}
        URL ${URL_STR}
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

# función para el cmakelists, para copiar las dlls al lado del exe
function(copy_sherpa_assets)
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

    # IMPORTANTE para Linux: Configurar RPATH para que el ejecutable encuentre las .so al lado del binario
    if (UNIX)
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
endfunction()