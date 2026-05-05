# ------------------------------------------------------------------------------
# Toolchain CycloneDDS + C++ Binding
# ------------------------------------------------------------------------------


# ==============================================================================
# Rutas globales
# ==============================================================================

include(ExternalProject)

set(CYCLONE_TOTAL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/cyclonedds-install")
set(CYCLONE_SRC_DIR           "${EXTERNAL_LIB_PATH}/cyclonedds_src")
set(CYCLONECPP_SRC_DIR        "${EXTERNAL_LIB_PATH}/cycloneddscpp_src")
set(IDL_GENERATED_DIR         "${CMAKE_SOURCE_DIR}/IDL/cyclone_generated")

# Detectar "formato" de librería
if(WIN32 AND MSVC)
    # Windows con compilador de Windows MSVC
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddsc.lib")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddscxx.lib")
else()
    # MinGW/Clang descargan librerías '.a'
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddsc.a")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddscxx.a")
endif()

# Establecer ejecutable de idl generator
if(WIN32)
    set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc.exe")
else()
    set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc")
endif()


# El configure necesita que la carpeta exista previamente
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include")
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx")

# ==============================================================================
# Configuración de generadores para ExternalProject_Add (generator_args)
# ==============================================================================

set(_generator_args -G "${CMAKE_GENERATOR}")
if(CMAKE_TOOLCHAIN_FILE)
    list(APPEND _generator_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

if(NOT MSVC)
    message(STATUS "Injecting C compiler to external project: ${CMAKE_C_COMPILER}")
    message(STATUS "Injecting CXX compiler to external project: ${CMAKE_CXX_COMPILER}")

    list(APPEND _generator_args
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DCLANG_PATH=${CLANG_PATH}"   # variable CMake, no de entorno
        "-DMINGW_PATH=${MINGW_PATH}"   # variable CMake, no de entorno
    )
endif()

# ==============================================================================
# Core de CycloneDDS
# ==============================================================================

# Probar esto para descarga persistente:
#    DOWNLOAD_COMMAND ""   # 👈 clave
#    UPDATE_COMMAND ""     # opcional


# Comprueba previamente si está instalado
if(EXISTS "${_ddsc_lib}")
    message(STATUS "CycloneDDS core ya instalado en ${CYCLONE_TOTAL_INSTALL_DIR}, saltando build")
    add_custom_target(cyclonedds_core)
else()
    message(STATUS "CycloneDDS core no encontrado, se compilará en el primer build")

    ExternalProject_Add(cyclonedds_core
        GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds.git
        GIT_TAG         11.0.1
        GIT_SHALLOW     TRUE
        SOURCE_DIR      "${CYCLONE_SRC_DIR}"
        INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"

        UPDATE_COMMAND  ""
        
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=$<CONFIG>"
            "-DBUILD_IDLC=ON"
            "-DENABLE_ICEORYX=OFF"
            "-DENABLE_SSL=OFF"
            "-DBUILD_EXAMPLES=OFF"
            "-DBUILD_TESTING=OFF"
            "-DBUILD_DDSPERF=OFF"
            ${_generator_args}

        BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG>
        INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target install
    )
endif()


# ==============================================================================
# Binding C++
# ==============================================================================

# Comprueba previamente si está instalado
if(EXISTS "${_ddscxx_lib}")
    message(STATUS "CycloneDDS C++ binding ya instalado, saltando build")
    add_custom_target(cyclonedds_cpp_binding)
else()
    ExternalProject_Add(cyclonedds_cpp_binding
        DEPENDS         cyclonedds_core
        GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
        GIT_TAG         11.0.1
        GIT_SHALLOW     TRUE
        SOURCE_DIR      "${CYCLONECPP_SRC_DIR}"
        INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"

        UPDATE_COMMAND  ""
        
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=$<CONFIG>"
            "-DCMAKE_PREFIX_PATH=${CYCLONE_TOTAL_INSTALL_DIR}"
            "-DBUILD_EXAMPLES=OFF"
            "-DBUILD_TESTING=OFF"

            "-DCycloneDDS_DIR=${CYCLONE_TOTAL_INSTALL_DIR}/lib/cmake/CycloneDDS"
            "-DCMAKE_PREFIX_PATH=${CYCLONE_TOTAL_INSTALL_DIR}"
            
            ${_generator_args}
        BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG>
        INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target install
    )
endif()


# ==============================================================================
# IDL — salida en IDL/cyclone_generated/, solo regenera si el .idl cambia
# ==============================================================================

# Carpeta de salida de cpp/hpp generado de .idl's
set(IDL_GENERATED_DIR         "${CMAKE_SOURCE_DIR}/IDL/cyclone_generated")
file(MAKE_DIRECTORY "${IDL_GENERATED_DIR}")

file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")
if(IDL_FILES)
    set(IDL_GENERATED_SOURCES "")
    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME "${IDL_FILE}" NAME_WE)
        set(_out_cpp "${IDL_GENERATED_DIR}/${IDL_NAME}.cpp")
        set(_out_hpp "${IDL_GENERATED_DIR}/${IDL_NAME}.hpp")

        add_custom_command(
            OUTPUT  "${_out_cpp}" "${_out_hpp}"
            COMMAND "${_idlc_exe}" -l cxx -o "${IDL_GENERATED_DIR}" "${IDL_FILE}"
            WORKING_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/bin"
            DEPENDS "${IDL_FILE}" cyclonedds_core cyclonedds_cpp_binding
            COMMENT "Generando C++ desde ${IDL_NAME}.idl"
            VERBATIM
        )
        list(APPEND IDL_GENERATED_SOURCES "${_out_cpp}")
    endforeach()
endif()


# ==============================================================================
# Creación de librerías intermedias 
# ==============================================================================

# Librería de cyclonedds -------------------------------------------------------
add_library(CycloneDDS::ddsc STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddsc PROPERTIES
    IMPORTED_LOCATION "${_ddsc_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddsc cyclonedds_core)

# Librería con cyclonedds_cxx --------------------------------------------------
add_library(CycloneDDS::ddscxx STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddscxx PROPERTIES
    IMPORTED_LOCATION "${_ddscxx_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddscxx cyclonedds_cpp_binding)

# Libreria con IDL's -----------------------------------------------------------
if(IDL_FILES)
    add_library(idl_generated_lib STATIC ${IDL_GENERATED_SOURCES})

    # Forzar C++17 para el binding moderno
    target_compile_features(idl_generated_lib PUBLIC cxx_std_17)

    # Añadir rutas de inclusión de la instalación de Cyclone
    target_include_directories(idl_generated_lib PUBLIC 
        "${IDL_GENERATED_DIR}"
        "${CYCLONE_TOTAL_INSTALL_DIR}/include"
        "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx"
    )

    target_link_libraries(idl_generated_lib PUBLIC CycloneDDS::ddscxx)
    add_dependencies(idl_generated_lib cyclonedds_core cyclonedds_cpp_binding)
endif()


# ==============================================================================
# Creación de librería global cycloneddscxx_lib con todo
# ==============================================================================

# Creamos una librería de interfaz
add_library(cycloneddscxx_lib INTERFACE)

# Enlazamos todas las partes:
target_link_libraries(cycloneddscxx_lib INTERFACE 
    idl_generated_lib
    CycloneDDS::ddsc
    CycloneDDS::ddscxx
)

# Propagación de las rutas de inclusión para poder hacer  
# #include <dds/dds.hpp> , #include "tu_idl.hpp", etc.
target_include_directories(cycloneddscxx_lib INTERFACE 
    "${IDL_GENERATED_DIR}"
    "${CYCLONE_TOTAL_INSTALL_DIR}/include"
    "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx"
    "${CYCLONE_TOTAL_INSTALL_DIR}/include/dds"
)

# Alias para que sea consistente con el uso de namespaces si se desea
add_library(CycloneDDS::all ALIAS cycloneddscxx_lib)

message(STATUS "Toolchain: Creado target agrupador 'cycloneddscxx_lib'")


# ==============================================================================
# Función para copiar DLLs al directorio de salida del ejecutable
# Usar en cmakelists principal después de generar el ejecutable
# ==============================================================================
function(configure_cyclonedds_dlls)
    if(WIN32)
        # Definimos las rutas de las DLLs en la carpeta install
        set(DLL_LIST 
            "${CYCLONE_TOTAL_INSTALL_DIR}/bin/ddsc.dll"
            "${CYCLONE_TOTAL_INSTALL_DIR}/bin/ddscxx.dll"
        )

        foreach(DLL_PATH ${DLL_LIST})
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copiando ${DLL_PATH} al directorio de salida..."
            )
        endforeach()
    endif()
endfunction()
