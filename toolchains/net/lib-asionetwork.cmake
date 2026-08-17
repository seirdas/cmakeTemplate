# -------------------------------
# Librería de asio-network con dependencias
# genera la librería asio_lib
# -------------------------------
include(FetchContent)
message(STATUS "[asio] Fetching asio-network library...")

# Versiones de librerías
set(ASIO_VERSION         asio-1-38-1)

# Versión para devolver al CMakeLists principal
set(LIB_VERSION ${ASIO_VERSION})


# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/asio_src/.git")
  message(STATUS "[asio] Library 'asio-network' found locally at: '${EXTERNAL_LIB_PATH}/asio_src'")
  set(FETCHCONTENT_SOURCE_DIR_ASIO_NETWORK
      "${EXTERNAL_LIB_PATH}/asio_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  asio_network
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        ${ASIO_VERSION}
  GIT_SHALLOW    TRUE        # habilita --depth 1
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/asio_src"
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(asio_network)

# Crear una librería estática de red
add_library(asio_lib STATIC "${asio_network_SOURCE_DIR}/src/asio.cpp")

# Incluir rutas de encabezado
target_include_directories(asio_lib SYSTEM PUBLIC "${asio_network_SOURCE_DIR}/include")

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
    bcrypt      # Biblioteca de criptografía de Windows (BCrypt* API);
  >    
)

# Omitir warnings de la propia librería
target_compile_options(asio_lib PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
      -Wno-shadow
      -Wno-unused-parameter
    >
)
