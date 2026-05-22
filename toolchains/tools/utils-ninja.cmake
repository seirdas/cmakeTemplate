
include(FetchContent)
cmake_policy(SET CMP0135 NEW) 

# Versión de ninja
set(NINJA_TAG "v1.13.2")
set(NINJA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/utils-ninja")

if(WIN32)
    set(NINJA_OS_SUFFIX "win")
    set(NINJA_BIN "ninja.exe")
elseif(UNIX)
    set(NINJA_OS_SUFFIX "linux")
    set(NINJA_BIN "ninja")
endif()

set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/${NINJA_TAG}/ninja-${NINJA_OS_SUFFIX}.zip")
set(NINJA_ZIP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ninja_${NINJA_OS_SUFFIX}.zip") # Ruta temporal del zip

# Comprobar si ya existe para no descargar
if (EXISTS "${NINJA_INSTALL_DIR}/${NINJA_BIN}")
    message(STATUS "[Ninja] Ninja found locally at: ${NINJA_INSTALL_DIR}")
else()
    message(STATUS "[Ninja] ${NINJA_BIN} not found. Downloading...")

    # Crear directorio
    file(MAKE_DIRECTORY "${NINJA_INSTALL_DIR}") 

    # Descargar el ZIP
    file(DOWNLOAD "${NINJA_URL}" "${NINJA_ZIP_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    # Extraer el archivo
    message(STATUS "[Ninja] Extracting...")
    file(ARCHIVE_EXTRACT
        INPUT "${NINJA_ZIP_PATH}"
        DESTINATION "${NINJA_INSTALL_DIR}"
    )

    # Borrar el ZIP descargado
    file(REMOVE "${NINJA_ZIP_PATH}")

    # Permisos de ejecución (Unix)
    if(UNIX)
        execute_process(COMMAND chmod +x "${NINJA_INSTALL_DIR}/${NINJA_BIN}")
    endif()
endif()

# Asegurar permisos de ejecución en linux
if(UNIX AND EXISTS "${NINJA_INSTALL_DIR}/${NINJA_BIN}")
    message(STATUS "[Ninja] Setting execution permissions for Ninja...")
    execute_process(COMMAND chmod +x "${NINJA_INSTALL_DIR}/ninja")
endif()

# Agregar al entorno
set(ENV{PATH} "${NINJA_INSTALL_DIR}:$ENV{PATH}")