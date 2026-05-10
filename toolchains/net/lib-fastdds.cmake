# ------------------------------------------------------------------------------
# Toolchain FastDDS
# ------------------------------------------------------------------------------

# =========================================================
# Configuración de Rutas para Fast-DDS Instalado
# =========================================================

# Ruta base (ajusta la versión si cambia)
set(EPROSIMA_INSTALL_DIR "C:/Program Files/eProsima/fastdds 3.2.2")
set(FASTDDSGEN_JAVA "${EPROSIMA_INSTALL_DIR}/share/fastddsgen/java/fastddsgen.jar")

# Dependencia openSSL (hay que instalarla previamente)
# https://slproweb.com/products/Win32OpenSSL.html
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
find_package(OpenSSL REQUIRED)

# Rutas de archivos .cmake al path de búsqueda
list(APPEND CMAKE_PREFIX_PATH 
    "${EPROSIMA_INSTALL_DIR}"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastcdr"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastdds"
)

# Buscamos los paquetes
find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)
find_package(foonathan_memory REQUIRED)

message(STATUS "[DDS] Fast-DDS encontrado en: ${fastdds_DIR}")


# =========================================================
# fastddsgen — Localizar ejecutable
# =========================================================

find_program(FASTDDSGEN_BIN 
    NAMES fastddsgen.bat fastddsgen 
    PATHS "${EPROSIMA_INSTALL_DIR}/bin"
    NO_DEFAULT_PATH
)
if(NOT FASTDDSGEN_BIN)
    message(FATAL_ERROR "No se encontró fastddsgen en la ruta de instalación.")
endif()


# =========================================================
# Generar hpp y cpp de idl's de la carpeta IDL
# =========================================================

# Carpeta de salida de cpp/hpp generado de .idl's
set(IDL_GENERATED_DIR         "${CMAKE_SOURCE_DIR}/IDL/fastdds_generated")
file(MAKE_DIRECTORY "${IDL_GENERATED_DIR}")

# Guardar todos los archivos IDL
file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")

set(IDL_GENERATED_SOURCES  "")

if(IDL_FILES)
    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        
        # Salida de FastDDS
        set(OUTPUTS
            "${IDL_GENERATED_DIR}/${IDL_NAME}.cxx"
            "${IDL_GENERATED_DIR}/${IDL_NAME}.h"
            "${IDL_GENERATED_DIR}/${IDL_NAME}PubSubTypes.cxx"
            "${IDL_GENERATED_DIR}/${IDL_NAME}PubSubTypes.h"
            "${IDL_GENERATED_DIR}/${IDL_NAME}TypeObjectSupport.cxx"  
            "${IDL_GENERATED_DIR}/${IDL_NAME}TypeObjectSupport.h"  
        )

        # java -jar "C:\Program Files\eProsima\fastdds 3.2.2\share\fastddsgen\java\fastddsgen.jar" -d . -replace "idl_data.idl" 
        add_custom_command(
            OUTPUT ${OUTPUTS}
            WORKING_DIRECTORY "${IDL_GENERATED_DIR}" 
            COMMAND java -jar ${FASTDDSGEN_JAVA} -d "${IDL_GENERATED_DIR}" -replace "${IDL_FILE}"
            DEPENDS ${IDL_FILE}
            COMMENT "Generando código DDS para ${IDL_NAME}..."
            VERBATIM
        )
        list(APPEND IDL_GENERATED_SOURCES  ${OUTPUTS})
    endforeach()
    
endif()


# =========================================================
# Librería interna con el código generado
# =========================================================

add_library(fastdds_idl_lib STATIC ${IDL_GENERATED_SOURCES})

target_link_libraries(fastdds_idl_lib PUBLIC
    fastdds
    fastcdr
)
target_include_directories(fastdds_idl_lib PUBLIC
        "${IDL_GENERATED_DIR}"
)
if(MSVC)
    target_compile_options(fastdds_idl_lib PRIVATE /W0 /wd5286 /wd5287)
endif()


# =========================================================
# fastdds_lib — librería de interfaz pública
# Agrupa: runtime FastDDS + OpenSSL + código IDL generado
# =========================================================
add_library(fastdds_lib INTERFACE)

target_link_libraries(fastdds_lib INTERFACE
    fastdds              # runtime + headers de eProsima
    fastcdr
    OpenSSL::SSL
    OpenSSL::Crypto
    fastdds_idl_lib      # código generado desde los IDL
)
target_include_directories(fastdds_lib INTERFACE
    "${EPROSIMA_INSTALL_DIR}/include"   # <fastdds/dds/...>
    "${IDL_GENERATED_DIR}"              # headers generados
)
target_compile_definitions(fastdds_lib INTERFACE
    USE_FASTDDS=1
)

message(STATUS "[FastDDS] fastdds_lib lista — IDL generados en: ${IDL_GENERATED_DIR}")