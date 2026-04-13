
include(FetchContent)
cmake_policy(SET CMP0135 NEW) 

# Versión de doxygen
set(DOXYGEN_TAG "Release_1_16_0")
set(DOXYGEN_VERSION "1.16.0")
set(DOXYGEN_INSTALL_DIR "${EXTERNAL_LIB_PATH}/utils-doxygen")

if(WIN32)
    set(DOXYGEN_EXECUTABLE "${DOXYGEN_INSTALL_DIR}/doxygen.exe")
    set(DOXYGEN_ASSET "doxygen-${DOXYGEN_VERSION}.x64.bin.zip")
elseif(UNIX)
    set(DOXYGEN_EXECUTABLE "${DOXYGEN_INSTALL_DIR}/bin/doxygen")
    set(DOXYGEN_ASSET "doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz")
endif()
set(DOXYGEN_URL "https://github.com/doxygen/doxygen/releases/download/${DOXYGEN_TAG}/${DOXYGEN_ASSET}")

# Comprobar si ya existe para no descargar
if (EXISTS "${DOXYGEN_EXECUTABLE}")
    message(STATUS "Doxygen found locally at: ${DOXYGEN_INSTALL_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_UTIL_DOXYGEN "${DOXYGEN_INSTALL_DIR}" CACHE PATH "" FORCE)
else()
    message(STATUS "Doxygen not found. Downloading...")
endif()

FetchContent_Declare(
    util_doxygen
    URL "${DOXYGEN_URL}"
    SOURCE_DIR "${DOXYGEN_INSTALL_DIR}"
)
FetchContent_MakeAvailable(util_doxygen)

# Asegurar permisos de ejecución en linux
if(UNIX)
    message(STATUS "Setting execution permissions for Doxygen...")
    execute_process(COMMAND chmod +x "${DOXYGEN_EXECUTABLE}")
endif()


# --- Lógica de limpieza ---
if(UNIX)
    message(STATUS "Cleaning up Doxygen extra folders...")

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