cmake_minimum_required(VERSION 3.10)

# Establecer estándar C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the project name from the root folder
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME CACHE)
project(${PROJECT_NAME})

# Make Windows headers use wide character set - COMPATIBLE CON MINGW
if(MSVC)
    # Para Visual Studio
    add_definitions(/D UNICODE /D _UNICODE)
else()
    # Para MinGW/GCC
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# Encontrar todos los archivos .cpp, .h y .hpp en la carpeta src
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")

# Crear el ejecutable a partir de los archivos .cpp
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

# Copia todo el contenido de la carpeta 'config' a la carpeta del binario
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Incluir la carpeta include para los archivos de cabecera
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Importar librerías externas
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Vincula la librería de hilos al ejecutable
if (NOT MSVC)
  # Encuentra la librería de hilos necesaria para GCC/MinGW
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
  # VINCULACIÓN ESTÁTICA de HILOS
  target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic)
  # VINCULACIÓN ESTÁTICA de C++ (STL) y GCC (Soporte de compilador)
  target_link_libraries(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

# Colocar los binarios por configuración en executable/{configuration}/ dentro del build dir
# Ej.: executable/Debug/ejecutable.exe
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
)
