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
set(BUILD_IDLC     ON  CACHE INTERNAL "")  # ON: necesario para compilar archivos .idl

# =======================
# Descarga
# =======================
FetchContent_Declare(
    cyclonedds
    GIT_REPOSITORY https://github.com/eclipse-cyclonedds/cyclonedds.git
    GIT_TAG        0.10.5      # Usa un tag concreto, no una rama, para builds reproducibles
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${CYCLONE_SRC_DIR}"
)
FetchContent_MakeAvailable(cyclonedds)

# =======================
# Compilación de archivos IDL
# =======================
# CycloneDDS SÍ incluye su propio compilador IDL (idlc) y una función CMake nativa.
# BUILD_IDLC ON es suficiente; después de FetchContent_MakeAvailable puedes usar:
#
#   idlc_generate(
#       TARGET    mi_idl_lib
#       FILES     mi_archivo.idl otro.idl
#   )
#   target_link_libraries(mi_target PRIVATE mi_idl_lib CycloneDDS::ddsc)
#
# idlc_generate crea automáticamente un target con los .c/.h generados.

# =======================
# Enlace (en tu target principal)
# =======================
# target_link_libraries(mi_target PRIVATE CycloneDDS::ddsc)