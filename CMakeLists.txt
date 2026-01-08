
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



# Librerías externas (necesita conexión a internet si no hay build)
# Se puede modificar la versión descargada en el GIT_TAG
# Ctrl+Click en link -> Tags (Verified)
include(FetchContent)
set(CMAKE_TLS_VERIFY OFF)

# glfw (Ventanas)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # <--- Desactiva boing, gears, etc.
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)    # <--- Desactiva los tests
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)     # <--- Desactiva la documentación
FetchContent_MakeAvailable(glfw)


# IMGUI (Interfaz)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_DIR)
file(GLOB IMGUI_SOURCES 
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)


# JSON (nlohmann/json)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)



# Encontrar todos los archivos .cpp, .h y .hpp en la carpeta src
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")



# Crear el ejecutable a partir de los archivos .cpp
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES} ${IMGUI_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()



target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL32)
# Incluir la carpeta include para los archivos de cabecera
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Librería de ImGui (GUI)
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${IMGUI_DIR} 
  ${IMGUI_DIR}/backends
  nlohmann_json::nlohmann_json
)

find_package(OpenGL REQUIRED)



# Vincula la librería de hilos al ejecutable (si no es MSVC)
if (NOT MSVC)
  # Vinculación de hilos, C++ (STL) y GCC (Soporte de compilador) para GCC/MinGW
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads
    -static-libgcc -static-libstdc++
    -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
  )
endif()



# Copia todo el contenido de la carpeta 'config' a la carpeta del binario
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)



# Colocar los binarios por configuración en executable/{configuration}/ dentro del build dir
# Ej.: executable/Debug/ejecutable.exe
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/$<CONFIG>"
)
