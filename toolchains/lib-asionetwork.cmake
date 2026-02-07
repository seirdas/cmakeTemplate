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

# declara el recurso externo que CMake descargará y hará disponible como una sub‑carpeta del proyecto
FetchContent_Declare(
    asio_network
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-36-0
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/asio_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(asio_network)

# Crear una librería estática de red + Vincular con Winsock2 en Windows
add_library(asio_lib INTERFACE)
target_include_directories(asio_lib INTERFACE "${asio_network_SOURCE_DIR}/asio/include")  # Incluir headers de Asio
target_compile_definitions(asio_lib INTERFACE ASIO_STANDALONE)                            # Definir ASIO_STANDALONE para usar Asio sin Boost
target_link_libraries(asio_lib INTERFACE
  $<$<PLATFORM_ID:Windows>:ws2_32 mswsock>  # Librerías de sockets en Windows
)     