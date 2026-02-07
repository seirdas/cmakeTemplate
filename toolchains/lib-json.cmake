# -------------------------------
# Librería de nlohmann/json (JSON) 
# -------------------------------

# Descarga el archivo json.hpp del último commit de github (si no existe)
# Se podría hacer también con el FetchContent pero tarda más
file(MAKE_DIRECTORY ${EXTERNAL_LIB_PATH}/nlohmann)
if (NOT EXISTS ${EXTERNAL_LIB_PATH}/nlohmann/json.hpp)
  message(STATUS "Downloading lastest release of nlohmann/json.hpp...")

  # Header-only: solo se necesita el header
  file(DOWNLOAD 
    "https://github.com/nlohmann/json/releases/latest/download/json.hpp"
    "${EXTERNAL_LIB_PATH}/nlohmann/json.hpp"
    STATUS DOWNLOAD_STATUS
  )
  if(NOT DOWNLOAD_STATUS EQUAL 0)
      message(FATAL_ERROR "Error downloading json.hpp")
  endif()
else()
  message(STATUS "nlohmann's json.hpp found")
endif()


# Crear una librería de interfaz para nlohmann/json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
  ${EXTERNAL_LIB_PATH}/nlohmann
)
