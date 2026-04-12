
include(FetchContent)
cmake_policy(SET CMP0135 NEW) 

# Versión de ninja
set(NINJA_VERSION "1.13.2")
set(NINJA_INSTALL_DIR "${EXTERNAL_LIB_PATH}/utils-ninja")

if(WIN32)
    set(NINJA_OS_SUFFIX "win")
    set(NINJA_EXE_EXT ".exe")
elseif(UNIX)
    set(NINJA_OS_SUFFIX "linux")
    set(NINJA_EXE_EXT "")
endif()

set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-${NINJA_OS_SUFFIX}.zip")

# Comprobar si ya existe para no descargar
if (EXISTS "${NINJA_INSTALL_DIR}/ninja${NINJA_EXE_EXT}")
    message(STATUS "Ninja found locally at: ${NINJA_INSTALL_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_UTIL_NINJA "${NINJA_INSTALL_DIR}" CACHE PATH "" FORCE)
else()
    message(STATUS "Ninja not found. Downloading...")
endif()

FetchContent_Declare(
    util_ninja
    URL "${NINJA_URL}"
    SOURCE_DIR "${NINJA_INSTALL_DIR}"
)
FetchContent_MakeAvailable(util_ninja)