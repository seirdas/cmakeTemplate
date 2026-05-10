# ------------------------------------------------------------------------------
# Toolchain FastDDS
# ------------------------------------------------------------------------------

# =========================================================
# Configuración de Rutas para Fast-DDS Instalado
# =========================================================

# 1. Definimos la ruta base (ajusta la versión si cambia)
set(EPROSIMA_INSTALL_DIR "C:/Program Files/eProsima/fastdds 3.2.2")

# Dependencia openSSL (hay que instalarla)
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
find_package(OpenSSL REQUIRED)

# 2. Añadimos las rutas de los archivos .cmake al path de búsqueda
list(APPEND CMAKE_PREFIX_PATH 
    "${EPROSIMA_INSTALL_DIR}"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastcdr"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastdds"
)

# 3. Buscamos los paquetes
find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)
find_package(foonathan_memory REQUIRED) # Suele venir incluido en la instalación

message(STATUS "[DDS] Fast-DDS encontrado en: ${fastdds_DIR}")

# =========================================================
# Compilador IDL
# =========================================================

# Localizar el ejecutable del generador instalado
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


set(FASTDDSGEN_JAVA "${EPROSIMA_INSTALL_DIR}/share/fastddsgen/java/fastddsgen.jar")

file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")

# Carpeta de salida de cpp/hpp generado de .idl's
set(IDL_GENERATED_DIR         "${CMAKE_SOURCE_DIR}/IDL/fastdds_generated")
file(MAKE_DIRECTORY "${IDL_GENERATED_DIR}")

message(STATUS "EPROSIMA_INSTALL_DIR: ${EPROSIMA_INSTALL_DIR}/share/fastddsgen/java/fastddsgen.jar")

# java -jar "C:\Program Files\eProsima\fastdds 3.2.2\share\fastddsgen\java\fastddsgen.jar" -d . -replace "${IDL_FILE}" 

if(IDL_FILES)
    set(GENERATED_SOURCES "")

    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        
        # IMPORTANTE: Definir exactamente lo que FastDDS-Gen escupe
        # Normalmente es ${IDL_NAME}.cxx y ${IDL_NAME}PubSubTypes.cxx
        set(OUTPUTS
            "${IDL_GENERATED_DIR}/${IDL_NAME}.cxx"
            "${IDL_GENERATED_DIR}/${IDL_NAME}.h"
            "${IDL_GENERATED_DIR}/${IDL_NAME}PubSubTypes.cxx"
            "${IDL_GENERATED_DIR}/${IDL_NAME}PubSubTypes.h"
            "${IDL_GENERATED_DIR}/${IDL_NAME}TypeObjectSupport.cxx"  
            "${IDL_GENERATED_DIR}/${IDL_NAME}TypeObjectSupport.h"  
        )


        add_custom_command(
            OUTPUT ${OUTPUTS}
            # Añadimos -pp (pre-processor) si es necesario y nos aseguramos del directorio
            COMMAND java -jar ${FASTDDSGEN_JAVA} -d "${IDL_GENERATED_DIR}" -replace "${IDL_FILE}"
            DEPENDS ${IDL_FILE}
            COMMENT "Generando código DDS para ${IDL_NAME}..."
            VERBATIM
        )
        list(APPEND GENERATED_SOURCES ${OUTPUTS})
    endforeach()

# Fix para evitar errores de archivos no encontrados durante la compilación paralela
set_source_files_properties(${GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

    add_library(idl_generated_lib STATIC ${GENERATED_SOURCES})
    
    # Enlazamos con los targets oficiales de la instalación
    target_link_libraries(idl_generated_lib PUBLIC fastdds fastcdr)
    
    target_include_directories(idl_generated_lib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

    if(MSVC)
        target_compile_options(idl_generated_lib PRIVATE /wd5286 /wd5287 /W0)
    endif()
endif()