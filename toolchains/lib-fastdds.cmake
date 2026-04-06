# -------------------------------
# Librería de Fast-DDS con dependencias
# genera la librería fastdds_lib
# -------------------------------

message(STATUS "Fetching Fast-DDS library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.github")
  message(STATUS "Using local fastdds source")
  set(FETCHCONTENT_SOURCE_DIR_ASIO_NETWORK
      "${EXTERNAL_LIB_PATH}/fastdds_src"
      CACHE PATH "" FORCE)
endif()

# Declarar dependencia externa para Fast-DDS
FetchContent_Declare(
    fastdds
    GIT_REPOSITORY https://github.com/eProsima/Fast-DDS.git
    GIT_TAG v3.5.0.0  # Puedes cambiar a una versión específica si es necesario
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastdds_src"
    EXCLUDE_FROM_ALL TRUE
)

# Hacerla disponible
FetchContent_MakeAvailable(fastdds)

# Crear una librería estática de Fast-DDS
add_library(fastdds_lib STATIC IMPORTED)
set_target_properties(fastdds_lib PROPERTIES
    IMPORTED_LOCATION "${fastdds_SOURCE_DIR}/lib/libfastdds.a"  
)

# Incluir directorios de Fast-DDS
target_include_directories(fastdds_lib INTERFACE "${fastdds_SOURCE_DIR}/src/cpp" "${fastdds_SOURCE_DIR}/include")