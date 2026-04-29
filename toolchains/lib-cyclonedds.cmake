# -------------------------------
# Librería CycloneDDS
# Target final: CycloneDDS::ddsc
# -------------------------------

include(FetchContent)

message(STATUS "Fetching CycloneDDS...")

# =======================
# Rutas locales
# =======================
set(CYCLONE_SRC_DIR "${EXTERNAL_LIB_PATH}/cyclonedds_src")

if(EXISTS "${CYCLONE_SRC_DIR}/.git")
    message(STATUS "Using local CycloneDDS source at ${CYCLONE_SRC_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_CYCLONEDDS "${CYCLONE_SRC_DIR}" CACHE PATH "" FORCE)
endif()

# =======================
# Opciones de compilación
# =======================
set(ENABLE_SSL     OFF CACHE INTERNAL "")
set(ENABLE_SHM     OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_TESTING  OFF CACHE INTERNAL "")
set(BUILD_DDSPERF  OFF CACHE INTERNAL "")
set(BUILD_IDLC     ON  CACHE INTERNAL "")  # ON: necesario para compilar archivos .idl
set(CYCLONEDDS_INSTALL_C_HEADERS OFF CACHE INTERNAL "")
set(CYCLONEDDS_INSTALL_CXX_HEADERS OFF CACHE INTERNAL "")
set(BUILD_CPP_BINDINGS ON CACHE INTERNAL "")
set(ENABLE_SECURITY OFF CACHE INTERNAL "")

# =======================
# Descarga
# =======================
FetchContent_Declare(
    cyclonedds
    GIT_REPOSITORY https://github.com/eclipse-cyclonedds/cyclonedds.git
    GIT_TAG        0.10.5      # Usa un tag concreto, no una rama, para builds reproducibles
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${CYCLONE_SRC_DIR}"
    EXCLUDE_FROM_ALL TRUE
    SYSTEM
)
FetchContent_MakeAvailable(cyclonedds)

# Omitir warnings de la librería
target_compile_options(ddsc PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W0            # Nivel de advertencia 0 (silencio total)
        /wd4244        # double a float
        /wd4305        # truncamiento de constantes
        /wd4267        # size_t a int
        /external:W0   # (CMake 3.22+) Silencia cabeceras externas
    >
    $<$<C_COMPILER_ID:MSVC>:
        /W0            # Nivel de advertencia 0 (silencio total)
        /wd4244        # double a float
        /wd4305        # truncamiento de constantes
        /wd4267        # size_t a int
        /external:W0   # (CMake 3.22+) Silencia cabeceras externas
    >
    
    # --- Configuración para GCC / Clang / MinGW ---
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -w             # Suprime todos los warnings
        -Wno-conversion
        -Wno-sign-compare
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-shadow
    >
)



# =======================
# Compilación de archivos IDL (en modo .c)
# idlc_generate crea automáticamente un target con los .c/.h generados.
# =======================

# Generar el "código" c y h a partir del IDL
idlc_generate(
    idl_generated_lib
    "${CMAKE_SOURCE_DIR}/IDL/idl_data.idl"
)

# Añade el .h generado al proyecto
target_include_directories(idl_generated_lib INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# También se vincula el propio cyclone al idl 
target_link_libraries(idl_generated_lib INTERFACE CycloneDDS::ddsc)
