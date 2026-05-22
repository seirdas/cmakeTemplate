# =======================
# Dependencia: FastCDR
# =======================
message(STATUS "[FastDDS] Fetching fastcdr library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastcdr_src/.git")
    message(STATUS "[FastDDS] Library 'fastcdr' found locally at: '${EXTERNAL_LIB_PATH}/fastcdr_src'")
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

message(STATUS "[FastDDS] Fetching foonathan_memory library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/foonathan_memory_src/.git")
    message(STATUS "[FastDDS] Library 'foonathan_memory' found locally at: '${EXTERNAL_LIB_PATH}/foonathan_memory_src'")
    set(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY "${EXTERNAL_LIB_PATH}/foonathan_memory_src" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY "https://github.com/foonathan/memory"
    GIT_TAG        v0.7-4
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

# FIX: foonathan_memory hereda los flags globales del proyecto (-Werror, -pedantic-errors...).
# En Clang/C++20 los literal operators con espacio (operator"" _KiB) son deprecated → error.
# Silenciamos TODOS los warnings en targets de terceros que no controlamos.
set(_foonathan_targets
    foonathan_memory
    foonathan_memory_tool   # se genera solo en algunos builds, protegemos con if
)
foreach(_t IN LISTS _foonathan_targets)
    if(TARGET ${_t})
        target_compile_options(${_t} PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:
                -Wno-deprecated-literal-operator
                -Wno-error=deprecated-literal-operator
                -Wno-error
            >
            $<$<CXX_COMPILER_ID:GNU>:
                -Wno-error
            >
            $<$<CXX_COMPILER_ID:MSVC>:
                /W0
            >
        )
        # Quitar pedantic-errors que viene del preset/toolchain global
        get_target_property(_opts ${_t} COMPILE_OPTIONS)
        if(_opts)
            list(REMOVE_ITEM _opts "-pedantic-errors")
            set_target_properties(${_t} PROPERTIES COMPILE_OPTIONS "${_opts}")
        endif()
    endif()
endforeach()

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

# -----------------------
# FIX PARA C++20 y -Werror
# -----------------------
foreach(target_name fastdds fastcdr)
    if(TARGET ${target_name})
        if (MSVC)
            target_compile_options(${target_name} PRIVATE
                /W0
            )
        else()
            target_compile_options(${target_name} PRIVATE
                -include cstdint
                -w
                -Wno-deprecated-literal-operator
                -Wno-error=deprecated-literal-operator
                -Wno-error
                -Wno-pedantic
            )
        endif()
    endif()
endforeach()
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

message(STATUS "[FastDDS] Fetching fastdds library...")
if(EXISTS "${EXTERNAL_LIB_PATH}/fastdds_src/.git")
    message(STATUS "[FastDDS] Library 'fastdds' found locally at: '${EXTERNAL_LIB_PATH}/fastdds_src'")
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

# Mismo fix para fastdds y fastcdr
foreach(_t IN LISTS fastdds fastcdr)
    if(TARGET ${_t})
        set_target_properties(fastdds PROPERTIES
            CXX_STANDARD    17
            CXX_STANDARD_REQUIRED OFF   # OFF = no error si el compilador no soporta 17
        )
        target_compile_options(${_t} PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang>: -Wno-error -w >
            $<$<CXX_COMPILER_ID:GNU>:              -Wno-error -w >
            $<$<CXX_COMPILER_ID:MSVC>:             /W0            >
        )
    endif()
endforeach()



# =========================================================
# Fast-DDS-Gen: Generación de código desde IDL
# =========================================================

# --- 1. Java (requisito de fastddsgen) ---
find_package(Java 11 REQUIRED COMPONENTS Runtime)
message(STATUS "[FastDDS-Gen] Java runtime found at: '${Java_JAVA_EXECUTABLE}'")

# --- 2. Descargar / localizar Fast-DDS-Gen ---
set(FASTDDSGEN_SRC "${EXTERNAL_LIB_PATH}/fastddsgen_src")

if(EXISTS "${FASTDDSGEN_SRC}/.git")
    message(STATUS "[FastDDS-Gen] Fast-DDS-Gen found locally at: '${FASTDDSGEN_SRC}'")
    set(FETCHCONTENT_SOURCE_DIR_FASTDDSGEN "${FASTDDSGEN_SRC}" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    fastddsgen
    GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS-Gen.git"
    GIT_TAG        v4.0.2
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${FASTDDSGEN_SRC}"
)
FetchContent_GetProperties(fastddsgen)

# --- 3. Compilar Fast-DDS-Gen con Gradle (solo si no está ya compilado) ---
if(WIN32)
    set(GRADLEW          "${FASTDDSGEN_SRC}/gradlew.bat")
    set(FASTDDSGEN_BIN   "${FASTDDSGEN_SRC}/scripts/fastddsgen.bat")
else()
    set(GRADLEW          "${FASTDDSGEN_SRC}/gradlew")
    set(FASTDDSGEN_BIN   "${FASTDDSGEN_SRC}/scripts/fastddsgen")
endif()

if(NOT EXISTS "${FASTDDSGEN_BIN}")
    if(NOT fastddsgen_POPULATED)
        FetchContent_Populate(fastddsgen)
    endif()

    if(NOT WIN32)
        execute_process(COMMAND chmod +x "${GRADLEW}")
    endif()

    message(STATUS "[FastDDS-Gen] Compiling Fast-DDS-Gen (Gradle assemble)...")
    execute_process(
        COMMAND             "${GRADLEW}" assemble
        WORKING_DIRECTORY   "${FASTDDSGEN_SRC}"
        RESULT_VARIABLE     GRADLE_RESULT
        OUTPUT_VARIABLE     GRADLE_OUTPUT
        ERROR_VARIABLE      GRADLE_OUTPUT
    )
    if(NOT GRADLE_RESULT EQUAL 0)
        message(FATAL_ERROR "[FastDDS-Gen] Gradle assemble falló:\n${GRADLE_OUTPUT}")
    endif()
    message(STATUS "[FastDDS-Gen] Fast-DDS-Gen compiled successfuly.")
else()
    message(STATUS "[FastDDS-Gen] Fast-DDS-Gen already compiled at: '${FASTDDSGEN_BIN}'")
endif()

# --- 4. Localizar todos los ficheros .idl ---
set(IDL_INPUT_DIR   "${CMAKE_SOURCE_DIR}/IDL")
set(IDL_OUTPUT_DIR  "${CMAKE_SOURCE_DIR}/IDL/idl_fastdds_generated")

file(MAKE_DIRECTORY "${IDL_OUTPUT_DIR}")
file(GLOB_RECURSE IDL_FILES "${IDL_INPUT_DIR}/*.idl")

if(NOT IDL_FILES)
    message(WARNING "[FastDDS-Gen] No se encontraron ficheros .idl en '${IDL_INPUT_DIR}'")
endif()

# --- 5. Regla de generación por cada .idl ---
set(IDL_GENERATED_SOURCES "")  # acumulará todos los .cxx/.hpp generados

foreach(IDL_FILE ${IDL_FILES})
    get_filename_component(IDL_NAME     "${IDL_FILE}" NAME_WE)  # sin extensión
    get_filename_component(IDL_ABS      "${IDL_FILE}" ABSOLUTE)

    # fastddsgen genera estos cuatro ficheros por cada IDL
    set(GEN_HEADER    "${IDL_OUTPUT_DIR}/${IDL_NAME}.hpp")
    set(GEN_SOURCE    "${IDL_OUTPUT_DIR}/${IDL_NAME}.cxx")
    set(GEN_PUBSUB_H  "${IDL_OUTPUT_DIR}/${IDL_NAME}PubSubTypes.hpp")
    set(GEN_PUBSUB_C  "${IDL_OUTPUT_DIR}/${IDL_NAME}PubSubTypes.cxx")

    set(IDL_GEN_OUTPUTS
        "${GEN_HEADER}"
        "${GEN_SOURCE}"
        "${GEN_PUBSUB_H}"
        "${GEN_PUBSUB_C}"
    )

    add_custom_command(
        OUTPUT          ${IDL_GEN_OUTPUTS}
        COMMAND         "${Java_JAVA_EXECUTABLE}"
                            -jar "${FASTDDSGEN_SRC}/share/fastddsgen/java/fastddsgen.jar"
                            -replace           # sobreescribir si ya existe
                            -d "${IDL_OUTPUT_DIR}"
                            "${IDL_ABS}"
        DEPENDS         "${IDL_ABS}"
        COMMENT         "[FastDDS-Gen] Generando código para ${IDL_NAME}.idl"
        VERBATIM
    )

    list(APPEND IDL_GENERATED_SOURCES ${IDL_GEN_OUTPUTS})
endforeach()

# --- 6. Target que agrupa toda la generación ---
add_custom_target(dds_idl_generate ALL
    DEPENDS ${IDL_GENERATED_SOURCES}
    COMMENT "[FastDDS-Gen] Todos los IDL generados."
)

# --- 7. Librería con el código generado (lista para linkear) ---
add_library(dds_idl_generated STATIC
    ${IDL_GENERATED_SOURCES}
)
add_dependencies(dds_idl_generate dds_idl_generated)

target_include_directories(dds_idl_generated PUBLIC
    "${IDL_OUTPUT_DIR}"
)
target_link_libraries(dds_idl_generated PUBLIC
    fastdds
    fastcdr
)

# Suprimir warnings en código autogenerado (no es tuyo, no lo toques)
target_compile_options(dds_idl_generated PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -Wno-unused-parameter
        -Wno-pedantic
        -Wno-extra
    >
)