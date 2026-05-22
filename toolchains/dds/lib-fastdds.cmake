# ------------------------------------------------------------------------------
# Toolchain FastDDS
# ------------------------------------------------------------------------------

# Versión para devolver al CMakeLists principal
set(LIB_VERSION 3.2.2)

# =========================================================
# Configuración de rutas para Fast-DDS Instalado
# =========================================================

# Ruta base (ajustar si cambia)
set(EPROSIMA_INSTALL_DIR "C:/Program Files/eProsima/fastdds 3.2.2")

# FastDDSGen (Generador idl's)
set(FASTDDSGEN_JAVA "${EPROSIMA_INSTALL_DIR}/share/fastddsgen/java/fastddsgen.jar")
if(NOT FASTDDSGEN_JAVA)
    message(FATAL_ERROR "[FastDDS] fastddsgen not found.")
endif()

# Dependencia openSSL (hay que instalarla previamente, y definir la ruta)
# https://slproweb.com/products/Win32OpenSSL.html
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
find_package(OpenSSL REQUIRED)

# Rutas de archivos .cmake al path de búsqueda
list(APPEND CMAKE_PREFIX_PATH 
    "${EPROSIMA_INSTALL_DIR}"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastcdr"
    "${EPROSIMA_INSTALL_DIR}/lib/cmake/fastdds"
)

# Build en shared, dll
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

# Buscamos los paquetes
find_package(fastcdr REQUIRED)
message(STATUS "[FastDDS] Fast-DDS found in: ${fastdds_DIR}")
find_package(fastdds REQUIRED)
message(STATUS "[FastDDS] FastCDR found in: ${fastcdr_DIR}")
find_package(foonathan_memory REQUIRED)
message(STATUS "[FastDDS] foonathan_memory found in: ${foonathan_memory_DIR}")


# =========================================================
# Generar hpp y cpp de idl's de la carpeta IDL
# =========================================================

# Carpeta de salida de cpp/hpp generado de .idl's
set(IDL_DIR         "${CMAKE_SOURCE_DIR}/IDL")
set(IDL_GENERATED_FOLDER       "fastdds_generated")
set(IDL_GENERATED_PATH         "${IDL_DIR}/${IDL_GENERATED_FOLDER}")
file(MAKE_DIRECTORY "${IDL_GENERATED_PATH}")

# Guardar todos los archivos IDL en IDL_FILES
file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")

# Crear la variable donde se almacenará los archivos de salida de fastddsgen
set(IDL_GENERATED_SOURCES  "")

# Generar cxx, hpp... de todos los idl's de la carpeta IDL
if(IDL_FILES)
    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        
        # Salida de FastDDS
        set(OUTPUTS
            "${IDL_GENERATED_PATH}/${IDL_NAME}.hpp"
            "${IDL_GENERATED_PATH}/${IDL_NAME}CdrAux.hpp"
            "${IDL_GENERATED_PATH}/${IDL_NAME}CdrAux.ipp"
            "${IDL_GENERATED_PATH}/${IDL_NAME}PubSubTypes.cxx"
            "${IDL_GENERATED_PATH}/${IDL_NAME}PubSubTypes.hpp"
            "${IDL_GENERATED_PATH}/${IDL_NAME}TypeObjectSupport.cxx"  
            "${IDL_GENERATED_PATH}/${IDL_NAME}TypeObjectSupport.hpp"  
        )

        # java -jar "C:\Program Files\eProsima\fastdds 3.2.2\share\fastddsgen\java\fastddsgen.jar" -d . -replace "idl_data.idl" 
        add_custom_command(
            OUTPUT ${OUTPUTS}
            WORKING_DIRECTORY "${IDL_GENERATED_PATH}" 
            COMMAND java -jar ${FASTDDSGEN_JAVA} -d "${IDL_GENERATED_PATH}" -replace "${IDL_FILE}"
            DEPENDS ${IDL_FILE}
            COMMENT "[FastDDSGen] Generating DDS files for ${IDL_NAME}..."
            VERBATIM
        )
        list(APPEND IDL_GENERATED_SOURCES  ${OUTPUTS})
    endforeach()


    # -----------------------------------
    # Generar archivo hpp genérico que incluya todos los generados
    # -----------------------------------
    # Buscar todos los .hpp de la carpeta
    file(GLOB_RECURSE GENERATED_HEADERS CONFIGURE_DEPENDS "${IDL_GENERATED_PATH}/*.hpp")
    set(UMBRELLA_FILE_NAME "_ALL.hpp")
    set(UMBRELLA_FILE "${IDL_GENERATED_PATH}/${UMBRELLA_FILE_NAME}")
    
    # Borrar el anterior si existe
    if (EXISTS ${UMBRELLA_FILE})
        file(REMOVE ${UMBRELLA_FILE})
    endif()
    
    # Primera línea del archivo
    set(UMBRELLA_CONTENT "// --- Archivo auto-generado desde CMake FastDDS toolchain ---\n#pragma once\n\n")

    # Iterar y añadir cada include
    foreach(HEADER_PATH ${GENERATED_HEADERS})
        get_filename_component(HEADER_NAME ${HEADER_PATH} NAME)
        if(NOT ${HEADER_NAME} STREQUAL ${UMBRELLA_FILE_NAME})
            string(APPEND UMBRELLA_CONTENT "#include \"${IDL_GENERATED_FOLDER}/${HEADER_NAME}\"\n")
        endif()
    endforeach()
    
    # Escribir todo en el archivo
    file(WRITE ${UMBRELLA_FILE} ${UMBRELLA_CONTENT})

endif()


# =========================================================
# Librería interna con el código generado
# =========================================================

# Crear una librería con los archivos generados
add_library(fastdds_idl_lib STATIC ${IDL_GENERATED_SOURCES})

# Vincular con las librerías de fastdds
target_link_libraries(fastdds_idl_lib PUBLIC
    fastdds
    fastcdr
)

# Añado la carpeta superior para evitar conflictos con los archivos generados por otras librerías (cyclone)
# Para incluir los archivos generados por fastdds, hay que añadir la carpeta, por ejemplo:
# #include "fastdds_generated/idl_dataCdrAux.hpp"
target_include_directories(fastdds_idl_lib PUBLIC
    "${IDL_DIR}"
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
)