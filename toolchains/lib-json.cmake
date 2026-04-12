# -------------------------------
# Librería de nlohmann/json (JSON) 
# -------------------------------

include(FetchContent)

# Version de json
set(JSON_VERSION "v3.12.0")
set(JSON_INSTALL_DIR "${EXTERNAL_LIB_PATH}/nlohmann_json")

# Configurar FetchContent
FetchContent_Declare(
    nlohmann_json
    URL "https://github.com/nlohmann/json/releases/download/${JSON_VERSION}/include.zip"
    SOURCE_DIR "${JSON_INSTALL_DIR}"
)

# Comprobar si ya existe localmente para evitar reconexiones innecesarias
if(NOT EXISTS "${JSON_INSTALL_DIR}/include/nlohmann/json.hpp")
    message(STATUS "nlohmann/json not found. Downloading version ${JSON_VERSION}...")
    FetchContent_MakeAvailable(nlohmann_json)
else()
    message(STATUS "Using local nlohmann/json from: ${JSON_INSTALL_DIR}")
endif()

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${JSON_INSTALL_DIR}/single_include")