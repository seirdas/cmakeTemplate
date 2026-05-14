
include(FetchContent)
cmake_policy(SET CMP0135 NEW) 

# Versión de doxygen
set(DOXYGEN_TAG "Release_1_16_0")
set(DOXYGEN_VERSION "1.16.0")
set(DOXYGEN_INSTALL_DIR "${EXTERNAL_LIB_PATH}/utils-doxygen")

if(WIN32)
    set(DOXYGEN_BIN "${DOXYGEN_INSTALL_DIR}/doxygen.exe")
    set(DOXYGEN_ZIP "doxygen-${DOXYGEN_VERSION}.x64.bin.zip")
elseif(UNIX)
    set(DOXYGEN_BIN "${DOXYGEN_INSTALL_DIR}/doxygen-${DOXYGEN_VERSION}/bin/doxygen")
    set(DOXYGEN_ZIP "doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz")
endif()

set(DOXYGEN_URL "https://github.com/doxygen/doxygen/releases/download/${DOXYGEN_TAG}/${DOXYGEN_ZIP}")
set(DOXYGEN_TEMP_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/doxygen.zip") # Ruta temporal del zip

# Comprobar si ya existe
if (EXISTS "${DOXYGEN_BIN}")
    message(STATUS "[Doxygen] Doxygen found locally at: ${DOXYGEN_BIN}")
else()
    message(STATUS "[Doxygen] Doxygen not found. Downloading from ${DOXYGEN_URL}...")

    # Crear directorio
    file(MAKE_DIRECTORY "${DOXYGEN_INSTALL_DIR}") 

    # Descargar ZIP
    file(DOWNLOAD "${DOXYGEN_URL}" "${DOXYGEN_TEMP_ARCHIVE}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )

    # Extraer el archivo
    message(STATUS "[Doxygen] Extracting Doxygen...")
    file(ARCHIVE_EXTRACT
        INPUT "${DOXYGEN_TEMP_ARCHIVE}"
        DESTINATION "${DOXYGEN_INSTALL_DIR}"
    )
    endif()

    # Borrar el ZIP descargado
    if (EXISTS ${DOXYGEN_TEMP_ARCHIVE})
        file(REMOVE "${DOXYGEN_TEMP_ARCHIVE}")
    endif()

    # Permisos de ejecución (Unix)
    if(UNIX AND EXISTS "${DOXYGEN_BIN}")
        execute_process(COMMAND chmod +x "${DOXYGEN_BIN}")
endif()

# --- Lógica de limpieza ---
if(UNIX)
    message(STATUS "[Doxygen] Cleaning up Doxygen extra folders...")

    set(TO_REMOVE
        "${DOXYGEN_INSTALL_DIR}/examples"
        "${DOXYGEN_INSTALL_DIR}/html"
        "${DOXYGEN_INSTALL_DIR}/man"
        "${DOXYGEN_INSTALL_DIR}/doxygen_manual-${DOXYGEN_VERSION}.pdf"
        "${DOXYGEN_INSTALL_DIR}/INSTALL"
        "${DOXYGEN_INSTALL_DIR}/Makefile"
    )

    foreach(ITEM ${TO_REMOVE})
        if(EXISTS ${ITEM})
            file(REMOVE_RECURSE ${ITEM})
        endif()
    endforeach()
endif()

# Solo genera doxygen en Release
function(generate_doxygen)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        
        COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> 
                $<$<CONFIG:Release>:-E> 
                $<$<CONFIG:Release>:echo> 
                $<$<CONFIG:Release>:"---- Generating Doxygen documentation...">
        
        # Ejecución silenciosa: añadimos QUIET=YES para mutear el log
        COMMAND $<$<CONFIG:Release>:${DOXYGEN_EXECUTABLE}> 
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile>
                $<$<CONFIG:Release>:QUIET=YES>
        
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
endfunction()