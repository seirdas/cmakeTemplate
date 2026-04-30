# ------------------------------------------------------------------------------
# Toolchain CycloneDDS + C++ Binding
# ------------------------------------------------------------------------------
include(ExternalProject)

set(CYCLONE_TOTAL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/cyclonedds-install")
set(CYCLONE_SRC_DIR           "${EXTERNAL_LIB_PATH}/cyclonedds_src")
set(CYCLONECPP_SRC_DIR        "${EXTERNAL_LIB_PATH}/cycloneddscpp_src")

set(_generator_args "")
if(CMAKE_TOOLCHAIN_FILE)
    list(APPEND _generator_args -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}")
endif()

# 1. PASO 1: Core de CycloneDDS
ExternalProject_Add(cyclonedds_core
    GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds.git
    GIT_TAG         11.0.1
    GIT_SHALLOW     TRUE
    SOURCE_DIR      "${CYCLONE_SRC_DIR}"
    INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"
    CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DBUILD_IDLC=ON"
        "-DENABLE_ICEORYX=OFF"
        "-DENABLE_SSL=OFF"
        "-DBUILD_EXAMPLES=OFF"
        "-DBUILD_TESTING=OFF"
        "-DBUILD_DDSPERF=OFF"
        ${_generator_args}
    BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
)

# 2. PASO 2: Binding C++ (Instalado en la misma ruta)
ExternalProject_Add(cyclonedds_cpp_binding
    DEPENDS         cyclonedds_core
    GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
    GIT_TAG         11.0.1
    GIT_SHALLOW     TRUE
    SOURCE_DIR      "${CYCLONECPP_SRC_DIR}"
    INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"
    CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_PREFIX_PATH=${CYCLONE_TOTAL_INSTALL_DIR}"
        "-DBUILD_EXAMPLES=OFF"
        "-DBUILD_TESTING=OFF"
        ${_generator_args}
    BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
)

# 3. Targets Importados

# El configure necesita que la carpeta exista previamente
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include")

# Librerías según plataforma
if(WIN32)
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddsc.lib")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddscxx.lib")
else()
    set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddsc.a")
    set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddscxx.a")
endif()

add_library(CycloneDDS::ddsc STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddsc PROPERTIES
    IMPORTED_LOCATION "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddsc.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddsc cyclonedds_core)

add_library(CycloneDDS::ddscxx STATIC IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddscxx PROPERTIES
    IMPORTED_LOCATION "${CYCLONE_TOTAL_INSTALL_DIR}/lib/ddscxx.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddscxx cyclonedds_cpp_binding)

# 4. Generación y Compilación de IDLs
set(IDLC_EXECUTABLE "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc.exe")
set(IDL_GENERATED_DIR "${CMAKE_BINARY_DIR}/idl_generated")
file(MAKE_DIRECTORY "${IDL_GENERATED_DIR}")

file(GLOB IDL_FILES "${CMAKE_SOURCE_DIR}/IDL/*.idl")

if(IDL_FILES)
    set(IDL_GENERATED_SOURCES "")
    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME "${IDL_FILE}" NAME_WE)
        set(_out_cpp "${IDL_GENERATED_DIR}/${IDL_NAME}.cpp")
        set(_out_hpp "${IDL_GENERATED_DIR}/${IDL_NAME}.hpp")

        add_custom_command(
            OUTPUT  "${_out_cpp}" "${_out_hpp}"
            COMMAND "${IDLC_EXECUTABLE}" -l cxx -o "${IDL_GENERATED_DIR}" "${IDL_FILE}"
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

    # IMPORTANTE: Forzar C++17 para el binding moderno
    target_compile_features(idl_generated_lib PUBLIC cxx_std_17)

    # Añadir rutas de inclusión de la instalación de Cyclone
    message(STATUS "CYCLONE_TOTAL_INSTALL_DIR: ${CYCLONE_TOTAL_INSTALL_DIR}")
    target_include_directories(idl_generated_lib PUBLIC 
        "${IDL_GENERATED_DIR}"
        "${CYCLONE_TOTAL_INSTALL_DIR}/include"
        "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx"
    )

    target_link_libraries(idl_generated_lib PUBLIC CycloneDDS::ddscxx)
endif()