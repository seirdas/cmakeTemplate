# =======================
# Dependencia: FastCDR
# =======================
message(STATUS "[DDS] Fetching fastcdr library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastcdr_src/.git")
    message(STATUS "[DDS] Library 'fastcdr' found locally at: '${EXTERNAL_LIB_PATH}/fastcdr_src'")
    set(FETCHCONTENT_SOURCE_DIR_FASTCDR "${EXTERNAL_LIB_PATH}/fastcdr_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    fastcdr
    GIT_REPOSITORY "https://github.com/eProsima/Fast-CDR"
    GIT_TAG        v2.2.1
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastcdr_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastcdr)

# FIX: fastcdrConfig.cmake se genera en tiempo de BUILD, no de CONFIGURE.
# find_package() interno de FastDDS no lo encuentra en disco todavía.
# Creamos un config sintético mínimo; el target 'fastcdr' ya está en scope
# porque FetchContent_MakeAvailable hizo add_subdirectory.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/fastcdr")
file(WRITE "${CMAKE_BINARY_DIR}/cmake/fastcdr/fastcdrConfig.cmake"
[[
set(fastcdr_FOUND TRUE)
]])
set(fastcdr_DIR "${CMAKE_BINARY_DIR}/cmake/fastcdr" CACHE PATH "" FORCE)


# =======================
# Dependencia: foonathan_memory
# =======================
set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FOONATHAN_MEMORY_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)

message(STATUS "[DDS] Fetching foonathan_memory library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/foonathan_memory_src/.git")
    message(STATUS "[DDS] Library 'foonathan_memory' found locally at: '${EXTERNAL_LIB_PATH}/foonathan_memory_src'")
    set(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY "${EXTERNAL_LIB_PATH}/foonathan_memory_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY "https://github.com/foonathan/memory"
    GIT_TAG        v0.7-3
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/foonathan_memory_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(foonathan_memory)

# Mismo problema que con fastcdr: config generado en build-time
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/foonathan_memory")
file(WRITE "${CMAKE_BINARY_DIR}/cmake/foonathan_memory/foonathan_memoryConfig.cmake"
[[
set(foonathan_memory_FOUND TRUE)
]])
set(foonathan_memory_DIR "${CMAKE_BINARY_DIR}/cmake/foonathan_memory" CACHE PATH "" FORCE)


# =======================
# Dependencia: TinyXML2
# (Ya hay otro toolchain para esto)
# =======================
if(NOT TARGET tinyxml2_lib)
    # Aoprovechar el toolchain creado si existe
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toolchains/file/lib-xml")
        include(net/lib-xml) # Esto genera el target tinyxml2_lib
    else()
        if (EXISTS "${EXTERNAL_LIB_PATH}/tinyxml2_src/.git")
        message(STATUS "Library 'tinyxml2' found locally at: '${EXTERNAL_LIB_PATH}/tinyxml2_src'")
        set(FETCHCONTENT_SOURCE_DIR_TINYXML2
            "${EXTERNAL_LIB_PATH}/tinyxml2_src"
            CACHE PATH "" FORCE)
        endif()
        FetchContent_Declare(
            tinyxml2
            GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
            GIT_TAG        10.0.0      # Versión estable más reciente
            GIT_SHALLOW    TRUE        # habilita --depth 1
            SOURCE_DIR     "${EXTERNAL_LIB_PATH}/tinyxml2_src"
            EXCLUDE_FROM_ALL TRUE
        )
        FetchContent_MakeAvailable(tinyxml2)
    endif()
endif()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/tinyxml2")
file(WRITE "${CMAKE_BINARY_DIR}/cmake/tinyxml2/TinyXML2Config.cmake"
[[
set(TinyXML2_FOUND TRUE)
]])
set(TinyXML2_DIR "${CMAKE_BINARY_DIR}/cmake/tinyxml2" CACHE PATH "" FORCE)
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/cmake/tinyxml2")


# =======================
# Dependencia: asio
# (Ya hay otro toolchain para esto)
# =======================
if(NOT TARGET asio_lib)
    # Aoprovechar el toolchain creado si existe
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toolchains/net/lib-asionetwork")
        include(net/lib-asionetwork) # Esto genera el target asio_lib
    else()
        if (EXISTS "${EXTERNAL_LIB_PATH}/asio_src/.git")
        message(STATUS "Library 'asio-network' found locally at: '${EXTERNAL_LIB_PATH}/asio_src'")
        set(FETCHCONTENT_SOURCE_DIR_ASIO_NETWORK
            "${EXTERNAL_LIB_PATH}/asio_src"
            CACHE PATH "" FORCE)
        endif()
        FetchContent_Declare(
            asio_network
            GIT_REPOSITORY   https://github.com/chriskohlhoff/asio.git
            GIT_TAG          asio-1-36-0
            GIT_SHALLOW      TRUE        # habilita --depth 1
            SOURCE_DIR       "${EXTERNAL_LIB_PATH}/asio_src"
            EXCLUDE_FROM_ALL TRUE
        )
        FetchContent_MakeAvailable(asio_network)
    endif()
endif()
set(Asio_INCLUDE_DIR "${EXTERNAL_LIB_PATH}/asio_src/asio/include" CACHE PATH "" FORCE)


# =======================
# Core: FastDDS
# =======================
set(COMPILE_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CHECK_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(STRICT_REALTIME     OFF CACHE BOOL "" FORCE)
# Impedir que intente update de third parties (ya descargadas)
set(THIRDPARTY         OFF CACHE STRING "" FORCE)
set(THIRDPARTY_fastcdr OFF CACHE STRING "" FORCE)
set(THIRDPARTY_asio OFF CACHE STRING "" FORCE)

message(STATUS "[DDS] Fetching fastdds library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.git")
    message(STATUS "[DDS] Library 'fastdds' found locally at: '${EXTERNAL_LIB_PATH}/fastdds_src'")
    set(FETCHCONTENT_SOURCE_DIR_FASTDDS "${EXTERNAL_LIB_PATH}/fastdds_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    fastdds
    GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS"
    GIT_TAG        v3.2.1
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${EXTERNAL_LIB_PATH}/fastdds_src"
    EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(fastdds)