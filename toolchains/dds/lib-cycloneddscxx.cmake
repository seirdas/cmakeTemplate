# ==============================================================================
# Toolchain CycloneDDS + C++ Binding
# ==============================================================================
include(ExternalProject)
cmake_policy(SET CMP0111 NEW) # Permite apuntar a targets que aún no existen
cmake_policy(SET CMP0169 OLD) # Permite "usar" FetchContent_Populate

# Versión de cyclone
set(CYCLONE_VERSION 11.0.1)

# Versión para devolver al CMakeLists principal
set(LIB_VERSION ${CYCLONE_VERSION})


# ==============================================================================
# Rutas globales
# ==============================================================================
set(CYCLONE_TOTAL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/cyclonedds-install")
set(CYCLONE_SRC_DIR           "${EXTERNAL_LIB_PATH}/cyclonedds_src")
set(CYCLONECPP_SRC_DIR        "${EXTERNAL_LIB_PATH}/cycloneddscpp_src")
set(IDL_GENERATED_PATH         "${CMAKE_SOURCE_DIR}/IDL/cyclone_generated")

# Detectar "formato" de librería
if(WIN32)
    # Establecer ejecutable de idl generator
    set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc.exe")

    # Establecer librerías (REVISAR)
    if(MSVC)  # Windows con compilador de Windows MSVC
        # Nombre de la librería
        set(_ddsc_lib_name   ddsc.lib)
        set(_ddscxx_lib_name ddscxx.lib)
    else()
        # MinGW/Clang descargan librerías '.a'
        set(_ddsc_lib_name   libddsc.a)
        set(_ddscxx_lib_name libddscxx.a)
    endif()
elseif(UNIX)
        # Establecer ejecutable de idl generator
        set(_idlc_exe   "${CYCLONE_TOTAL_INSTALL_DIR}/bin/idlc")

        # Nombre de la librería
        set(_ddsc_lib_name   libddsc.so.${CYCLONE_VERSION})
        set(_ddscxx_lib_name libddscxx.so.${CYCLONE_VERSION})

endif()


# Ruta de librería
set(_ddsc_lib   "${CYCLONE_TOTAL_INSTALL_DIR}/lib/${_ddsc_lib_name}")
set(_ddscxx_lib "${CYCLONE_TOTAL_INSTALL_DIR}/lib/${_ddscxx_lib_name}")

# El configure necesita que la carpeta exista previamente
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include")
file(MAKE_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx")


# ==============================================================================
# Configuración por compilador de generadores para ExternalProject_Add (generator_args)
# ==============================================================================

set(_generator_args -G "${CMAKE_GENERATOR}")
if(CMAKE_TOOLCHAIN_FILE)
    list(APPEND _generator_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

# Detectar "formato" de librería
if(WIN32)
    # Windows
    if(MSVC)
        # Windows con compilador de Windows MSVC

    elseif(MINGW)
        # MinGW/Clang 
        message(STATUS "[Cyclone] Injecting C compiler to external project: ${CMAKE_C_COMPILER}")
        message(STATUS "[Cyclone] Injecting CXX compiler to external project: ${CMAKE_CXX_COMPILER}")
        list(APPEND _generator_args
            "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        )

        # Fix para UNICODE
        list(APPEND _generator_args
            "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -UUNICODE -U_UNICODE"
            "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -UUNICODE -U_UNICODE"
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Solo Windows Clang
            list(APPEND _generator_args "-DCLANG_PATH=${CLANG_PATH}" )

        else()
            # Solo Windows MinGW
            list(APPEND _generator_args "-DMINGW_PATH=${MINGW_PATH}" )

        endif()
    endif()
elseif(UNIX)
    # Linux

endif()


# ==============================================================================
# CycloneDDS Core
# ==============================================================================

# ---------------------------------------------
# Descargar previamente el código fuente en src
# ---------------------------------------------
if (EXISTS "${CYCLONE_SRC_DIR}/.github")
  message(STATUS "[Cyclone] CycloneDDS source found locally at: '${CYCLONE_SRC_DIR}'")
  set(FETCHCONTENT_SOURCE_DIR_CYCLONEDDS_FETCH
      "${CYCLONE_SRC_DIR}"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    cyclonedds_fetch
    GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds
    GIT_TAG         ${CYCLONE_VERSION}
    GIT_SHALLOW     TRUE
    SOURCE_DIR      "${CYCLONE_SRC_DIR}"
    GIT_PROGRESS    TRUE
)
FetchContent_Populate(cyclonedds_fetch)

# ---------------------------------------------
# Instrucciones de compilación e instalación de librería
# ---------------------------------------------
if(EXISTS "${_ddsc_lib}")
    message(STATUS "[Cyclone] Cyclone core lib found locally at: '${_ddsc_lib}'")
    add_custom_target(cyclonedds_core)
else()
    message(STATUS "[Cyclone] CycloneDDS core not found, it will be built at first build.")

    ExternalProject_Add(cyclonedds_core
        SOURCE_DIR      "${CYCLONE_SRC_DIR}"
        INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"

        # Fase de descarga anulada (ya lo hizo el Populate)
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND  ""
        
        # Ver documentación en el readme de la URL de github
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=$<CONFIG>"

            "-DBUILD_EXAMPLES=OFF"
            "-DBUILD_TESTING=OFF"
            
            "-DBUILD_IDLC=ON"
            "-DBUILD_DDSPERF=OFF"          
            
            "-DENABLE_SSL=OFF"
            "-DENABLE_ICEORYX=OFF"
            # "-DENABLE_SECURITY=NO"
            # "-DENABLE_LIFESPAN=NO"
            # "-DENABLE_DEADLINE_MISSED=NO"
            # "-DENABLE_TYPELIB=ON"           # disabled requires also disabling type and topic discovery
            # "-DENABLE_TYPE_DISCOVERY=NO"
            # "-DENABLE_TOPIC_DISCOVERY=NO"
            # "-DENABLE_SOURCE_SPECIFIC_MULTICAST=NO"
            # "-DENABLE_IPV6=NO"
            # "-DBUILD_IDLC_XTESTS=NO"
            # "-DENABLE_QOS_PROVIDER=NO"

            # "-DCMAKE_DISABLE_FIND_PACKAGE_CUnit=TRUE"     # No existe esto  
            # "-DBUILD_XTESTS=OFF"                          # No existe esto
            ${_generator_args}

        BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG>
        INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target install
        BUILD_BYPRODUCTS "${_ddsc_lib}" "${_idlc_exe}"
    )
endif()


# ==============================================================================
# CycloneDDS C++ Binding
# ==============================================================================

# ---------------------------------------------
# Descargar previamente el código fuente en src
# ---------------------------------------------
if (EXISTS "${CYCLONECPP_SRC_DIR}/.github")
  message(STATUS "[Cyclone] CycloneDDS C++ Binding source found locally at: '${CYCLONECPP_SRC_DIR}'")
  set(FETCHCONTENT_SOURCE_DIR_CYCLONEDDS_CXX_FETCH
      "${CYCLONECPP_SRC_DIR}"
      CACHE PATH "" FORCE)
endif()
FetchContent_Declare(
    cyclonedds_cxx_fetch
    GIT_REPOSITORY  https://github.com/eclipse-cyclonedds/cyclonedds-cxx
    GIT_TAG         ${CYCLONE_VERSION}
    GIT_SHALLOW     TRUE
    SOURCE_DIR      "${CYCLONECPP_SRC_DIR}"
    GIT_PROGRESS    TRUE
)
FetchContent_Populate(cyclonedds_cxx_fetch)

# ---------------------------------------------
# Instrucciones de compilación e instalación de librería
# ---------------------------------------------
if(EXISTS "${_ddscxx_lib}")
    message(STATUS "[Cyclone] Cyclone c++ binding lib found locally at: '${_ddscxx_lib}'")
    add_custom_target(cyclonedds_cpp_binding)
else()
    message(STATUS "[Cyclone] CycloneDDS C++ binding not found, it will be built at first build.")
    ExternalProject_Add(cyclonedds_cpp_binding
        DEPENDS         cyclonedds_core
        SOURCE_DIR      "${CYCLONECPP_SRC_DIR}"
        INSTALL_DIR     "${CYCLONE_TOTAL_INSTALL_DIR}"

        # Fase de descarga anulada (ya lo hizo el Populate)
        DOWNLOAD_COMMAND ""
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
        BUILD_BYPRODUCTS "${_ddscxx_lib}"
    )
endif()


# ==============================================================================
# IDL — salida en IDL/cyclone_generated/, solo regenera si el .idl cambia
# ==============================================================================

# Carpeta de salida de cpp/hpp generado de .idl's
set(IDL_DIR         "${CMAKE_SOURCE_DIR}/IDL")
set(IDL_GENERATED_FOLDER       "cyclone_generated")
set(IDL_GENERATED_PATH         "${IDL_DIR}/${IDL_GENERATED_FOLDER}")
file(MAKE_DIRECTORY "${IDL_GENERATED_PATH}")

file(GLOB IDL_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/IDL/*.idl")

# Generar cxx, hpp... de todos los idl's de la carpeta IDL
if(IDL_FILES)

    # Crear la variable donde se almacenará los archivos de salida de fastddsgen
    set(IDL_GENERATED_SOURCES "")
    
    # Archivo hpp que contiene todos los demás
    set(UMBRELLA_FILE_NAME "_ALL.hpp")
    set(UMBRELLA_FILE "${IDL_GENERATED_PATH}/${UMBRELLA_FILE_NAME}")
    if (EXISTS ${UMBRELLA_FILE})
        file(REMOVE ${UMBRELLA_FILE})
    endif()
    set(UMBRELLA_CONTENT "// --- Archivo auto-generado desde CMake CycloneDDS toolchain ---\n#pragma once\n\n")

    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME "${IDL_FILE}" NAME_WE)

        # Salida de CycloneDDS
        set(_out_cpp "${IDL_GENERATED_PATH}/${IDL_NAME}.cpp")
        set(_out_hpp "${IDL_GENERATED_PATH}/${IDL_NAME}.hpp")

        # Procesar IDL
        add_custom_command(
            OUTPUT  "${_out_cpp}" "${_out_hpp}"
            COMMAND "${_idlc_exe}" -l cxx -o "${IDL_GENERATED_PATH}" "${IDL_FILE}"
            WORKING_DIRECTORY "${CYCLONE_TOTAL_INSTALL_DIR}/bin"
            DEPENDS "${IDL_FILE}" cyclonedds_core cyclonedds_cpp_binding
            COMMENT "Generando C++ desde ${IDL_NAME}.idl"
            VERBATIM
        )
        list(APPEND IDL_GENERATED_SOURCES "${_out_cpp}")

        # Escribir en el archivo de hpp's globales
        string(APPEND UMBRELLA_CONTENT "#include \"${IDL_GENERATED_FOLDER}/${IDL_NAME}.hpp\"\n")
    endforeach()

    set(UMBRELLA_FILE "${IDL_GENERATED_PATH}/_ALL.hpp")
    file(WRITE "${UMBRELLA_FILE}" "${UMBRELLA_CONTENT}")

endif()


# ==============================================================================
# Creación de librerías intermedias 
# ==============================================================================

# Librería de cyclonedds -------------------------------------------------------
add_library(CycloneDDS::ddsc SHARED IMPORTED GLOBAL)
set_target_properties(CycloneDDS::ddsc PROPERTIES
    IMPORTED_LOCATION "${_ddsc_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${CYCLONE_TOTAL_INSTALL_DIR}/include"
)
add_dependencies(CycloneDDS::ddsc cyclonedds_core)


# Librería con cyclonedds_cxx --------------------------------------------------
add_library(CycloneDDS::ddscxx SHARED IMPORTED GLOBAL)
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
    # Añado la carpeta superior para evitar conflictos con los archivos generados por otras librerías (fastdds)
    # Para incluir los archivos generados por cyclone, hay que añadir la carpeta, por ejemplo:
    # #include "cyclone_generated/idl_data.hpp"
    target_include_directories(idl_generated_lib PUBLIC 
        "${IDL_DIR}"
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
    CycloneDDS::ddscxx
    CycloneDDS::ddsc
)

# Propagación de las rutas de inclusión para poder hacer  
# #include <dds/dds.hpp> , #include "tu_idl.hpp", etc.
target_include_directories(cycloneddscxx_lib SYSTEM INTERFACE 
    "${IDL_GENERATED_PATH}"
    "${CYCLONE_TOTAL_INSTALL_DIR}/include"
    "${CYCLONE_TOTAL_INSTALL_DIR}/include/ddscxx"
)

# Alias para que sea consistente con el uso de namespaces si se desea
add_library(CycloneDDS::all ALIAS cycloneddscxx_lib)

# Ajustes Linux
if(UNIX)
    # Buscar soporte de hilos en el sistema
  find_package(Threads REQUIRED)
  
  # Linkado de dependencias base de Linux
  target_link_libraries(cycloneddscxx_lib INTERFACE
    Threads::Threads  # Reemplaza a winpthread de Windows
    dl                # Necesario para ImGui/OpenGL/Carga dinámica
    m                 # Librería matemática (Miniaudio, etc.)
  )
endif()

message(STATUS "[Cyclone] Cyclone: All targets grouped into 'cycloneddscxx_lib'")


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
    elseif(UNIX)
        set(DLL_LIST "")
        file(GLOB_RECURSE DLL_LIST            
            "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libdds*"
            "${CYCLONE_TOTAL_INSTALL_DIR}/lib/libddscxx*"
        )
    endif()
    
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND @echo ---- Linking Cyclone dlls to output folder...
        )

        foreach(DLL_PATH ${DLL_LIST})
            # Obtener solo el nombre del archivo (ej: libddscxx.so o ddscxx.dll)
            get_filename_component(DLL_NAME "${DLL_PATH}" NAME)

            # Log en build de lo que va a hacer...
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND @echo ${DLL_PATH} ↔ $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )

            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DLL_NAME}"
                VERBATIM
            )
        endforeach()

    if (UNIX)
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    endif()

endfunction()
