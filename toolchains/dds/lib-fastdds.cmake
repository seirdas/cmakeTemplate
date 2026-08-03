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

# Dependencia openSSL (hay que instalarla previamente, borrar cmake caché, y definir la ruta)
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
set(IDL_DIR         "${CMAKE_SOURCE_DIR}/interfaces/IDL")
set(IDL_GENERATED_FOLDER       "fastdds_generated")
set(IDL_GENERATED_PATH         "${IDL_DIR}/${IDL_GENERATED_FOLDER}")
file(MAKE_DIRECTORY "${IDL_GENERATED_PATH}")

# Guardar todos los archivos IDL en IDL_FILES
file(GLOB IDL_FILES CONFIGURE_DEPENDS "${IDL_DIR}/*.idl")

# Generar cxx, hpp... de todos los idl's de la carpeta IDL
if(IDL_FILES)

    # Crear la variable donde se almacenará los archivos de salida de fastddsgen
    set(IDL_GENERATED_SOURCES  "")

    # Archivo hpp que contiene todos los demás
    set(UMBRELLA_FILE_NAME "_ALL.hpp")
    set(UMBRELLA_FILE "${IDL_GENERATED_PATH}/${UMBRELLA_FILE_NAME}")
    if (EXISTS ${UMBRELLA_FILE})
        file(REMOVE ${UMBRELLA_FILE})
    endif()
    set(UMBRELLA_CONTENT "// --- Archivo auto-generado desde CMake FastDDS toolchain ---\n#pragma once\n\n")

    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        
        # Salida de FastDDS
        set(OUTPUTS
            "${IDL_NAME}.hpp"
            "${IDL_NAME}CdrAux.hpp"
            "${IDL_NAME}CdrAux.ipp"
            "${IDL_NAME}PubSubTypes.cxx"
            "${IDL_NAME}PubSubTypes.hpp"
            "${IDL_NAME}TypeObjectSupport.cxx"  
            "${IDL_NAME}TypeObjectSupport.hpp"  
        )

        # Añade ruta absoluta "C:/.../IDL/fastdds_generated/" al principio de cada elemento
        set(OUTPUTS_ABSOLUTE_PATH ${OUTPUTS})
        list(TRANSFORM OUTPUTS_ABSOLUTE_PATH PREPEND "${IDL_GENERATED_PATH}/")

        # Guardar ruta relativa para el archivo de hpps globales
        set(OUTPUTS_RELATIVE_PATH ${OUTPUTS})
        list(TRANSFORM OUTPUTS_RELATIVE_PATH PREPEND "${IDL_GENERATED_FOLDER}/")

        # Procesar IDL
        # java -jar "C:\Program Files\eProsima\fastdds 3.2.2\share\fastddsgen\java\fastddsgen.jar" -d . -replace "idl_data.idl" 
        add_custom_command(
            OUTPUT ${OUTPUTS_ABSOLUTE_PATH}
            WORKING_DIRECTORY "${IDL_GENERATED_PATH}" 
            COMMAND java -jar ${FASTDDSGEN_JAVA} -d "${IDL_GENERATED_PATH}" -replace "${IDL_FILE}"
            DEPENDS ${IDL_FILE}
            COMMENT "[FastDDSGen] Generating DDS files for ${IDL_NAME}..."
            VERBATIM
        )

        # Guardar los cpp's generados
        set(COMPILE_SOURCES ${OUTPUTS_ABSOLUTE_PATH})
        list(FILTER COMPILE_SOURCES EXCLUDE REGEX "\\.ipp$")
        list(APPEND IDL_GENERATED_SOURCES ${COMPILE_SOURCES})

        # Escribir en el archivo de hpp's globales
        list(FILTER OUTPUTS_RELATIVE_PATH INCLUDE REGEX "\\.hpp$")
        foreach(HEADER_NAME ${OUTPUTS})
            string(APPEND UMBRELLA_CONTENT "#include \"${IDL_GENERATED_FOLDER}/${HEADER_NAME}\"\n")
        endforeach()

    endforeach()
    
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

# Creamos una librería de interfaz
add_library(fastdds_lib INTERFACE)

# Enlazamos todas las partes:
target_link_libraries(fastdds_lib INTERFACE
    fastdds              # runtime + headers de eProsima
    fastcdr
    OpenSSL::SSL
    OpenSSL::Crypto
    fastdds_idl_lib      # código generado desde los IDL
)

# Propagación de las rutas de inclusión
target_include_directories(fastdds_lib INTERFACE
    "${EPROSIMA_INSTALL_DIR}/include"   # <fastdds/dds/...>
)

# =================================
#   Funcion de copia de dlls a la salida
# =================================

function(configure_fastdds_deps)

    if (ARCH_NAME STREQUAL "x64")
        set(FASTDDS_BIN_FOLDER "x64Win64VS2019")
    else()
        set(FASTDDS_BIN_FOLDER "i86Win32VS2019")
    endif()

    set(FASTDDS_BIN_DIR "${EPROSIMA_INSTALL_DIR}/bin/${FASTDDS_BIN_FOLDER}")

    # Lista de dependencias
    set(DLL_LIST
        "libcrypto-3-x64.dll"
        "libssl-3-x64.dll"
        "OpenSSL.Fast-DDS.manifest"
    )

    # Añadir las dependencias correspondientes según si es Debug o Release
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR (MSVC AND NOT CMAKE_CONFIGURATION_TYPES MATCHES "Release"))
        # Si es entorno o configuración de Debug
        list(APPEND DLL_LIST
            "fastcdrd-2.3.dll"
            "fastddsd-3.2.dll"
            "foonathan_memory-0.7.3-dbg.dll"
        )
    else()
        # Si es Release, RelWithDebInfo, MinSizeRel, etc.
        list(APPEND DLL_LIST
            "fastcdr-2.3.dll"
            "fastdds-3.2.dll"
            "foonathan_memory-0.7.3.dll"
        )
    endif()

    # Log de lo que se va a hacer...
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "---- Copying FastDDS dependencies to output folder..."
    )

    # Copiar las dlls al lado del ejecutable
    foreach(DLL_NAME ${DLL_LIST})
        set(SRC_PATH "${FASTDDS_BIN_DIR}/${DLL_NAME}")

        # Log en build de lo que va a hacer...
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "${SRC_PATH} --- $<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # Nos movemos a la carpeta del EXE para que el link sea local (sin slashes)
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}" "${DLL_NAME}"
            VERBATIM
        )
    endforeach()

endfunction()