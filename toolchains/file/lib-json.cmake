# -------------------------------
# Librería de nlohmann/json (JSON) 
# -------------------------------
include(FetchContent)
message(STATUS "[json] Fetching nlohmann/json library...")

# Versión de json
set(JSON_VERSION "v3.12.0")

# Versión para devolver al CMakeLists principal
set(LIB_VERSION ${JSON_VERSION})

# Ruta externa
set(JSON_INSTALL_DIR "${EXTERNAL_LIB_PATH}/json_release")

# Configurar FetchContent
cmake_policy(SET CMP0135 NEW) 
fetchcontent_declare(
    nlohmann_json
    URL "https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/include.zip"
    SOURCE_DIR "${JSON_INSTALL_DIR}"
)

# Comprobar si ya existe localmente para evitar reconexiones innecesarias
if(NOT EXISTS "${JSON_INSTALL_DIR}/include/nlohmann/json.hpp")
    message(STATUS "[json] nlohmann/json not found. Downloading version ${JSON_VERSION}...")
    fetchcontent_makeavailable(nlohmann_json)
else()
    message(STATUS "[json] Using local nlohmann/json header from: ${JSON_INSTALL_DIR}")
endif()

add_library(nlohmann_json INTERFACE)

# Usar SYSTEM INTERFACE para que MSVC ignore warnings de cabeceras de terceros
target_include_directories(nlohmann_json SYSTEM INTERFACE "${JSON_INSTALL_DIR}/single_include")

# Desactivar explícitamente el warning C5045 para cualquier target que enlace esta librería
if(MSVC)
    target_compile_options(nlohmann_json INTERFACE /wd5045)
endif()


# Gestión de licencia
if(EXISTS "${THIRD_PARTY_LICENSES_FILE}")
    set(LIB_LICENSE_PATH "${JSON_INSTALL_DIR}/LICENSE.MIT")
    
    if(EXISTS "${LIB_LICENSE_PATH}")
        file(READ "${LIB_LICENSE_PATH}" LIB_LICENSE_TEXT)
        
        file(APPEND "${THIRD_PARTY_LICENSES_FILE}"
            "------------------------------------------------------------------------\n"
            " Library: nlohmann/json (Version: ${LIB_VERSION})\n"
            "------------------------------------------------------------------------\n\n"
            "${LIB_LICENSE_TEXT}\n\n"
        )
    else()
      message(WARNING "License file not found in: ${LIB_LICENSE_PATH}")
    endif()
endif()
