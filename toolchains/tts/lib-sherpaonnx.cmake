include(FetchContent)
message(STATUS "[Sherpa] Fetching sherpa onnx tts...")
cmake_policy(SET CMP0135 NEW)

# ----------------------------------------------------------------------------
# Selección de librería sherpa
# ----------------------------------------------------------------------------

# Versión de Sherpa
set(SHERPA_VERSION "1.13.4")

# Versión para devolver al CMakeLists principal
set(LIB_VERSION ${SHERPA_VERSION})

option(USE_SHERPA_WIN_GPU   "Compilar con soporte Windows GPU"  ON)      # Activar/desactivar librería compatible con GPU (Windows)
option(USE_CUDA             "Sherpa con soporte CUDA"           OFF)     # Activar/desactivar copiar la dll pesada de CUDA


# ----------------------------------------------------------------------------
# Configuración específica del SO
# ----------------------------------------------------------------------------

# Identificar arquitectura de compilación
if(NOT DEFINED ARCH_NAME)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "[Sherpa] Targeting 64-bit architecture.")
        set(ARCH_NAME "x64" CACHE STRING "Target Architecture")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(STATUS "[Sherpa] Targeting 32-bit architecture.")
        set(ARCH_NAME "x86" CACHE STRING "Target Architecture")
    endif()
endif()

if(WIN32)
	# Directorio de descarga y libreria objetivo
    set(SHERPA_INSTALL_PATH "${EXTERNAL_LIB_PATH}/sherpa_win_release")  # Ojo, este "release" es el del github, no el de la configuración

    # Opciones para Windows - build MD CPU específicas por config, no hay GPU disponible
    set(SHERPA_WIN_DEBUG            "sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-Debug" )
    set(SHERPA_WIN_RELEASE          "sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-Release" )
    set(SHERPA_WIN_MINSIZEREL       "sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-MinSizeRel" )
    set(SHERPA_WIN_RELWITHDEBINFO   "sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-RelWithDebInfo" )

    # Obtener ruta de librería según CPU/GPU y x64/x32
    if (USE_SHERPA_WIN_GPU)
        set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/lib")
    else()
        # Se "repite" porque $<CONFIG> solo se lee en build, no en configure
        set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-$<CONFIG>/lib")
    endif()

    # Versión compatible con GPU (DirectML, aunque no lo ponga), hace también fallback a CPU si falla, solo x64
    set(SHERPA_WIN_BIN_GPU          "sherpa-onnx-v${SHERPA_VERSION}-win-x64-cuda")

    # Urls
    set(SHERPA_WIN_URL_GPU            "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_BIN_GPU}.tar.bz2")
    set(SHERPA_WIN_URL_DEBUG          "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_DEBUG}.tar.bz2")
    set(SHERPA_WIN_URL_RELEASE        "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_RELEASE}.tar.bz2")
    set(SHERPA_WIN_URL_MINSIZEREL     "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_MINSIZEREL}.tar.bz2")
    set(SHERPA_WIN_URL_RELWITHDEBINFO "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_WIN_RELWITHDEBINFO}.tar.bz2")

    # Librerías necesarias
    set(SHERPA_CHECK_LIBS
        sherpa-onnx-c-api.lib
        sherpa-onnx-cxx-api.lib
    )

elseif(UNIX)      # Linux x64. (NO ES COMPATIBLE CON x86)
    # Directorio de descarga y libreria objetivo
    set(SHERPA_INSTALL_PATH "${EXTERNAL_LIB_PATH}/sherpa_linux_release")

    # Nombre de carpeta/zip y paquete a descargar
    set(SHERPA_LINUX_BIN "sherpa-onnx-v${SHERPA_VERSION}-linux-x64-gpu" )

    # Obtener ruta de librería
    set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/lib")
    
    # Url
    set(SHERPA_LINUX_URL "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_VERSION}/${SHERPA_LINUX_BIN}.tar.bz2")

    # Librerías necesarias
    set(SHERPA_CHECK_LIBS
        libsherpa-onnx-c-api.so
        libsherpa-onnx-cxx-api.so
        libonnxruntime.so
        libonnxruntime_providers_shared.so
        libonnxruntime_providers_cuda.so
        libonnxruntime_providers_tensorrt.so
    )

    # Librerías necesarias
    set(SHERPA_CHECK_LIBS libsherpa-onnx-c-api.so)

endif()

# Crear el directorio de descarga
file(MAKE_DIRECTORY "${SHERPA_INSTALL_PATH}")

# ----------------------------------------------------------------------------
# Función de descarga (para usar justo después)
# ----------------------------------------------------------------------------
function(download_sherpa SHERPA_BIN URL)

    set(ALL_LIBS_EXIST TRUE)
    foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
        if (NOT EXISTS "${SHERPA_INSTALL_PATH}/${SHERPA_BIN}/lib/${LIB_NAME}")
            set(ALL_LIBS_EXIST FALSE)
            break()
        endif()
    endforeach()

    if (ALL_LIBS_EXIST)
        message(STATUS "[Sherpa] Sherpa-ONNX ${SHERPA_BIN}: All libs found locally.")
    else()
        message(STATUS "[Sherpa] Sherpa-ONNX ${SHERPA_BIN} missing components. Downloading...")
        # Descarga y descomprimir
        set(TEMP_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/${SHERPA_BIN}.tar.bz2")
        file(DOWNLOAD "${URL}" "${TEMP_ARCHIVE}" SHOW_PROGRESS)
        message(STATUS "[Sherpa] Extracting ${TEMP_ARCHIVE}...")
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
    if(EXISTS "${SHERPA_BIN_DIR}")
        message(STATUS "[Sherpa] Deleting ${SHERPA_BIN_DIR} (not used)")
        file(REMOVE_RECURSE "${SHERPA_BIN_DIR}")
    endif()

endfunction()


# Configurar según plataforma
if(WIN32)

    # Utilizar librería de GPU si está activado y si es x64
    if (USE_SHERPA_WIN_GPU AND ARCH_NAME STREQUAL "x64")
        message(STATUS "[Sherpa] Selected sherpa library with GPU compatibility")

        # Descargar y desplegar el paquete
        download_sherpa(${SHERPA_WIN_BIN_GPU}          ${SHERPA_WIN_URL_GPU})
        
        # Crear la librería de interfaz
        add_library(sherpa_lib INTERFACE)

        # Incluir headers
        target_include_directories(sherpa_lib SYSTEM INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/include")

        # Link de librerías
        foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
            target_link_libraries(sherpa_lib INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/lib/${LIB_NAME}")
        endforeach()
        target_link_libraries(sherpa_lib INTERFACE 
            ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
            winmm       # Bibliotecas de Multimedia de Windows
        )

    else()
        message(STATUS "[Sherpa] Selected sherpa library limited to CPU compatibility")

        # Aprovechamos y descargamos todas las configuraciones
        download_sherpa(${SHERPA_WIN_DEBUG}          ${SHERPA_WIN_URL_DEBUG})
        download_sherpa(${SHERPA_WIN_RELEASE}        ${SHERPA_WIN_URL_RELEASE})
        download_sherpa(${SHERPA_WIN_MINSIZEREL}     ${SHERPA_WIN_URL_MINSIZEREL})
        download_sherpa(${SHERPA_WIN_RELWITHDEBINFO} ${SHERPA_WIN_URL_RELWITHDEBINFO})

        # Crear la librería de interfaz
        add_library(sherpa_lib INTERFACE)

        # Incluir headers (Todos son iguales, cualquier config vale)
        target_include_directories(sherpa_lib SYSTEM INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_RELEASE}/include")

        # Link de librerías (se va a elegir la de la configuración correspondiente)
        foreach(LIB_NAME ${SHERPA_CHECK_LIBS})
            target_link_libraries(sherpa_lib INTERFACE
                "$<$<CONFIG:Debug>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_DEBUG}/lib/${LIB_NAME}>"
                "$<$<CONFIG:Release>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_RELEASE}/lib/${LIB_NAME}>"
                "$<$<CONFIG:MinSizeRel>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_MINSIZEREL}/lib/${LIB_NAME}>"
                "$<$<CONFIG:RelWithDebInfo>:${SHERPA_INSTALL_PATH}/${SHERPA_WIN_RELWITHDEBINFO}/lib/${LIB_NAME}>"
            )
        endforeach()
        target_link_libraries(sherpa_lib INTERFACE 
            ws2_32      # Winsock2 - API de sockets de Windows (prob. no necesario)
            winmm       # Bibliotecas de Multimedia de Windows
        )
    endif()


else()
    # Descarga el binario de Linux
    download_sherpa(${SHERPA_LINUX_BIN}          ${SHERPA_LINUX_URL})

    # Crear la librería
    add_library(sherpa_lib INTERFACE)

    # Incluir headers
    target_include_directories(sherpa_lib SYSTEM INTERFACE "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/include")

    # Link de librerías 
    set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_LINUX_BIN}/lib")
    target_link_libraries(sherpa_lib INTERFACE
        "${SHERPA_LIB_PATH}/libsherpa-onnx-c-api.so"
        "${SHERPA_LIB_PATH}/libsherpa-onnx-cxx-api.so"
        "${SHERPA_LIB_PATH}/libonnxruntime.so"
        "${SHERPA_LIB_PATH}/libonnxruntime_providers_shared.so"
        pthread
        dl
    )

    # Link de librerías GPU solo si se requiere
    if(USE_CUDA)
        target_link_libraries(sherpa_lib INTERFACE
            "${SHERPA_LIB_PATH}/libonnxruntime_providers_tensorrt.so"
            "${SHERPA_LIB_PATH}/libonnxruntime_providers_cuda.so"
        )
    endif()

endif()


# =================================
#   Funciones
# =================================

# función para el cmakelists, para copiar las dlls y assets en la ruta del exe
function(configure_sherpa_deps)

    # Identificar DLLs a copiar
    if(WIN32)

        # Lista de DLLs
        set(DLL_LIST 
            "sherpa-onnx-c-api.dll" 
            "sherpa-onnx-cxx-api.dll" 
            "onnxruntime.dll" 
            "onnxruntime_providers_shared.dll" 
        )

        # Obtener ruta de librería según CPU / GPU
        if (USE_SHERPA_WIN_GPU AND ARCH_NAME STREQUAL "x64")
            set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/${SHERPA_WIN_BIN_GPU}/lib")

            # Añadir dependencias de GPU CUDA si está activado
            if (USE_CUDA)
                list(APPEND DLL_LIST 
                    "onnxruntime_providers_cuda.dll"
                    "onnxruntime_providers_tensorrt.dll"
                )
            endif()
        else()
            set(SHERPA_LIB_PATH "${SHERPA_INSTALL_PATH}/sherpa-onnx-v${SHERPA_VERSION}-win-${ARCH_NAME}-shared-MD-$<CONFIG>/lib")
        endif()


    elseif(UNIX)

        # Lista de DLLs (.so)
        set(DLL_LIST 
            "libsherpa-onnx-c-api.so" 
            "libsherpa-onnx-cxx-api.so" 
            "libonnxruntime.so" 
            "libonnxruntime_providers_shared.so"
        )

        # Añadir dependencias de GPU CUDA si está activado
        if (USE_CUDA)
            list(APPEND DLL_LIST 
                "libonnxruntime_providers_cuda.so"
                "libonnxruntime_providers_tensorrt.so"
            )
        endif()

    endif()

    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "---- Linking Sherpa dlls to output folder..."
    )

    # Copiar las dlls al lado del ejecutable
    foreach(DLL_NAME ${DLL_LIST})
        set(SRC_PATH "${SHERPA_LIB_PATH}/${DLL_NAME}")

        # Log en build de lo que va a hacer...
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "${SRC_PATH} --- $<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # Nos movemos a la carpeta del EXE para que el link sea local (sin slashes)
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E create_hardlink "${SRC_PATH}" "${DLL_NAME}"
            VERBATIM
        )
    endforeach()

    # Los siguientes target_properties hay que hacerlos cuando el proyecto esté creado:

    # Linux: Configurar RPATH para que el ejecutable encuentre las .so al lado del binario
    if (UNIX AND NOT SHERPA_RPATH_CONFIGURED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
        set(SHERPA_RPATH_CONFIGURED ON CACHE INTERNAL "RPATH has been set for Sherpa")
    endif()

endfunction()
