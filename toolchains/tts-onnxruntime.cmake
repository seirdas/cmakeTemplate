# -------------------------------
# Motor de Inferencia ONNX Runtime (Versión Persistente)
# -------------------------------

include(FetchContent)

message(STATUS "Configuring onnx runtime...")


set(ONXX_VERSION "1.24.3")

# Configurar URLs según plataforma
if(WIN32)
    set(ONNX_INSTALL_DIR "${EXTERNAL_LIB_PATH}/onnxruntime_win_src")
    set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONXX_VERSION}/onnxruntime-win-x64-${ONXX_VERSION}.zip")
    set(ONNX_LIB "onnxruntime.lib")
else()
    set(ONNX_INSTALL_DIR "${EXTERNAL_LIB_PATH}/onnxruntime_linux_src")
    set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONXX_VERSION}/onnxruntime-linux-x64-${ONXX_VERSION}.tgz")
    set(ONNX_LIB "libonnxruntime.so")
endif()

# Usa la fuente local si ya existe
if (EXISTS "${ONNX_INSTALL_DIR}/lib/${ONNX_LIB}")
    message(STATUS "Using local onnxruntime_src source from ${EXTERNAL_LIB_PATH}/onnxruntime_src")
    set(FETCHCONTENT_SOURCE_DIR_BABYLON
        "${EXTERNAL_LIB_PATH}/onnxruntime_src" CACHE PATH "" FORCE
    )
else()
    message(STATUS "ONNX Runtime no encontrado en ${ONNX_INSTALL_DIR}. Descargando...")
    FetchContent_Declare(
        onnx_binaries
        URL ${ONNX_URL}
        SOURCE_DIR "${ONNX_INSTALL_DIR}"
    )
    FetchContent_MakeAvailable(onnx_binaries)
endif()

# Crear la librería de interfaz
add_library(onnxruntime_lib INTERFACE)

# Headers (siempre apuntando a la carpeta persistente)
target_include_directories(onnxruntime_lib INTERFACE "${ONNX_INSTALL_DIR}/include")

# Link de la librería (.lib / .so)
target_link_libraries(onnxruntime_lib INTERFACE "${ONNX_INSTALL_DIR}/lib/${ONNX_LIB}")

# Omitir warnings de la propia librería
target_compile_options(onnxruntime_lib INTERFACE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
      -Wno-shadow
      -Wno-unused-parameter
      -Wno-sign-compare
      -Wno-unused-variable
      -Wunused-but-set-variable
    >
)

# Define la función para copiar archivos y directorios 
# SE USA EN EL POST-BUILD
function(copy_onnx_runtime)
    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNX_INSTALL_DIR}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying ONNX Runtime DLL desde ${ONNX_INSTALL_DIR}..."
        )
    endif()
endfunction()