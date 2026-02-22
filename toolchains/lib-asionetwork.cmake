# -------------------------------
# Librería de asio-network con dependencias
# genera la librería asio_lib
# -------------------------------

include(FetchContent)

message(STATUS "Fetching asio-network library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/asio_src/.github")
  message(STATUS "Using local asio source")
  set(FETCHCONTENT_SOURCE_DIR_ASIO_NETWORK
      "${EXTERNAL_LIB_PATH}/asio_src"
      CACHE PATH "" FORCE)
endif()

# Declarar dependencia externa
FetchContent_Declare(
    asio_network
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-36-0
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/asio_src"
    EXCLUDE_FROM_ALL TRUE
)
# Hacerla disponible
FetchContent_MakeAvailable(asio_network)

# Crear una librería estática de red + Vincular con Winsock2 en Windows
add_library(asio_lib INTERFACE)
target_include_directories(asio_lib INTERFACE "${asio_network_SOURCE_DIR}/asio/include")  # Incluir headers de Asio

# Definir ASIO_STANDALONE para usar Asio sin Boost
target_compile_definitions(asio_lib INTERFACE 
  ASIO_STANDALONE
)

# Vincular con las librerías de sockets de Windows
target_link_libraries(asio_lib INTERFACE
  $<$<PLATFORM_ID:Windows>:ws2_32>     # Winsock2 - API de sockets de Windows
  $<$<PLATFORM_ID:Windows>:mswsock>    # Microsoft Winsock Extensions
  $<$<PLATFORM_ID:Windows>:wsock32>    # Winsock - API de sockets antigua (a veces requerida)
)

target_compile_options(asio_lib INTERFACE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-shadow>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unused-parameter>
)
