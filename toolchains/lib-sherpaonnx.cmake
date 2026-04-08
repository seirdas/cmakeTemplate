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
    # TODO
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
    )
    FetchContent_MakeAvailable(sherpa_pkg_${CONFIG_LOWER})
endfunction()

download_sherpa(Debug          ${URL_DEBUG})
download_sherpa(Release        ${URL_RELEASE})
download_sherpa(MinSizeRel     ${URL_MINSIZEREL})
download_sherpa(RelWithDebInfo ${URL_RELWITHDEBINFO})

# Crear la librería de interfaz
add_library(sherpa_lib INTERFACE)

# Headers: Usamos la de Release como referencia (son iguales en todas)
target_include_directories(sherpa_lib INTERFACE "${SHERPA_INSTALL_DIR}/release/include")

# Link de la librería (.lib / .so) por configuración
foreach(CONFIG Debug Release Minsizerel Relwithdebinfo)
    string(TOLOWER ${CONFIG} L_CONFIG)
    set(LIB_PATH "${SHERPA_INSTALL_DIR}/${L_CONFIG}/lib")
    
    target_link_libraries(sherpa_lib INTERFACE 
        $<$<CONFIG:${L_CONFIG}>:"${LIB_PATH}/sherpa-onnx-c-api.lib">
        $<$<CONFIG:${L_CONFIG}>:"${LIB_PATH}/onnxruntime.lib">
    )

    # TODO LINUX
endforeach()

if(WIN32)
    target_link_libraries(sherpa_lib INTERFACE ws2_32 winmm)
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


function(copy_sherpa_assets)
    # Detectar la configuración actual (Debug, Release, etc.)
    set(CURRENT_CONFIG_DIR "$<CONFIG>")
    string(TOLOWER "${CURRENT_CONFIG_DIR}" L_CONFIG)

    # A) Copiar DLLs necesarias
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SHERPA_INSTALL_DIR}/$<LOWER_CASE:$<CONFIG>>/lib/sherpa-onnx-c-api.dll"
        "${SHERPA_INSTALL_DIR}/$<LOWER_CASE:$<CONFIG>>/lib/onnxruntime.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying Sherpa and ONNX Runtime DLLs ($<CONFIG>)..."
    )
endfunction()