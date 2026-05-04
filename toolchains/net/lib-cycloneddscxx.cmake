# ------------------------------------------------------------------------------
# Toolchain CycloneDDS + C++ Binding
# ------------------------------------------------------------------------------


# ==============================================================================
# Configuración global
# ==============================================================================

include(ExternalProject)

set(CYCLONE_TOTAL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/cyclonedds-install")
set(CYCLONE_SRC_DIR           "${EXTERNAL_LIB_PATH}/cyclonedds_src")
set(CYCLONECPP_SRC_DIR        "${EXTERNAL_LIB_PATH}/cycloneddscpp_src")
set(IDL_GENERATED_DIR         "${CMAKE_SOURCE_DIR}/IDL/cyclone_generated")

# Detectar plataforma
if(WIN32)
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddsc.lib")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddscxx.lib")
    set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc.exe")
else()
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddsc.a")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddscxx.a")
    set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc")
endif()

set(_generator_args "")
if(CMAKE_TOOLCHAIN_FILE)
    list(APPEND _generator_args -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}")
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
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=$<CONFIG>"
            "-DCMAKE_PREFIX_PATH=${CYCLONE_TOTAL_INSTALL_DIR}"
            "-DBUILD_EXAMPLES=OFF"
            "-DBUILD_TESTING=OFF"
            ${_generator_args}
        BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG>
        INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target install
    )
endif()


# ==============================================================================
# Targets importados (siempre, independientemente de si se compiló ahora o antes)
# ==============================================================================

# El configure necesita que la carpeta exista previamente
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include")
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx")

# Crea una librería con cyclonedds
add_library(CycloneDDS::ddsc STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddsc PROPERTIES
    IMPORTED_LOCATION "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddsc.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddsc cyclonedds_core)

# Crea una librería con cyclonedds_cxx
add_library(CycloneDDS::ddscxx STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddscxx PROPERTIES
    IMPORTED_LOCATION "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddscxx.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddscxx cyclonedds_cpp_binding)


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

    # Asegurar que existe la carpeta para el configure
    file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include")
    file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx")

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
# Interfaz cycloneddscxx con todo
# ==============================================================================
# Creamos una librería de interfaz
add_library(cycloneddscxx_lib INTERFACE)

# Enlazamos todas las partes: 
# 1. idl_generated_lib (que ya trae el código de tus .idl y hereda CycloneDDS::ddscxx)
# 2. CycloneDDS::ddsc (el core de C, necesario para los símbolos de ddsrt, ddsi, etc.)
target_link_libraries(cycloneddscxx_lib INTERFACE 
    idl_generated_lib
    CycloneDDS::ddsc
    CycloneDDS::ddscxx
)

# Propagamos las rutas de inclusión para que el usuario final pueda hacer 
# #include <dds/dds.hpp> y #include "tu_idl.hpp" sin configurar nada más.
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
# ==========================================================
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
