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

FetchContent_Declare(
    asio_network
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-36-0
    GIT_SHALLOW    TRUE        # habilita --depth 1
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/asio_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(asio_network)

# Crear una librería estática de red
add_library(asio_lib STATIC "${asio_network_SOURCE_DIR}/asio/src/asio.cpp")

# Incluir rutas de encabezado
target_include_directories(asio_lib PUBLIC "${asio_network_SOURCE_DIR}/asio/include")

# Macros de compilación
target_compile_definitions(asio_lib PUBLIC 
  ASIO_STANDALONE                     # Asio sin Boost
  ASIO_DISABLE_SMALL_BLOCK_RECYCLING
  ASIO_NO_TS_EXECUTORS
  ASIO_SEPARATE_COMPILATION

  _WIN32_WINNT=0x0601       # Windows 7 o superior
  WINVER=0x0601             # Windows 7 o superior
  NTDDI_VERSION=0x06010000  # Windows 7 o superior
)

# Vincular con las librerías de sockets de Windows
target_link_libraries(asio_lib PUBLIC
  $<$<PLATFORM_ID:Windows>:
    ws2_32      # Winsock2 - API de sockets de Windows
    mswsock     # Microsoft Winsock Extensions
    wsock32     # Winsock - API de sockets antigua (a veces requerida)
    bcrypt      # 
  >    
)

# Omitir warnings de la propia librería
target_compile_options(asio_lib PUBLIC
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
      -Wno-shadow
      -Wno-unused-parameter
    >
)
