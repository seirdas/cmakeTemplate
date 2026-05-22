# -------------------------------
# Librería de nlohmann/json (JSON) 
# -------------------------------

include(FetchContent)
cmake_policy(SET CMP0135 NEW) 

message(STATUS "[json] Fetching nlohmann/json library...")

# Version de json
set(JSON_VERSION "v3.12.0")
set(JSON_INSTALL_DIR "${EXTERNAL_LIB_PATH}/json_release")

# Configurar FetchContent
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
target_include_directories(nlohmann_json INTERFACE "${JSON_INSTALL_DIR}/single_include")