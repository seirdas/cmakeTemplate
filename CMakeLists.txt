
# CMake minimum version and project setup
cmake_minimum_required(VERSION 3.10)
# Establecer estándar C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Set the project name from the root folder
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME CACHE)
project(${PROJECT_NAME})



# Librerías externas (necesita conexión a internet si no hay build)
# Se puede modificar la versión descargada en el GIT_TAG
# Ctrl+Click en link -> Tags (Verified)
include(FetchContent)

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
# Obligar a usar la librería de tiempo de ejecución dinámica en MSVC
FetchContent_MakeAvailable(glfw)
# Configurar runtime dinámico para glfw en MSVC
if(MSVC)
  set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
endif()



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



# Encontrar todos los archivos .cpp, .h y .hpp
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc")



# Crear el ejecutable a partir de los archivos .cpp
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES} ${IMGUI_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()



# Rutas de inclusión de librerías externas
target_link_libraries(${PROJECT_NAME} PRIVATE 
  glfw 
  OpenGL32
  nlohmann_json::nlohmann_json
)
find_package(OpenGL REQUIRED) # Requerido para ImGui con OpenGL
if(OpenGL_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()



# Rutas de archivos de cabecera del proyecto
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Librería de ImGui (GUI)
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${IMGUI_DIR} 
  ${IMGUI_DIR}/backends
  nlohmann_json::nlohmann_json
)



# Configuración específica para MSVC
if(MSVC)
  # Especifica el proyecto principal
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  # Usar el main normal aunque sea una App de Windows
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
  # Configurar runtime dinámico para el proyecto en MSVC
  set_target_properties(${PROJECT_NAME} PROPERTIES 
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
  )
  # Configurar runtime dinámico para glfw en MSVC
  set_target_properties(glfw PROPERTIES 
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
  )
  # Configurar GLFW para usar runtime dinámico en MSVC
  set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
  # 3. Librerías de sistema necesarias
  target_link_libraries(${PROJECT_NAME} PRIVATE 
      imm32.lib 
      dwmapi.lib 
      uxtheme.lib 
      version.lib 
      shell32.lib
      ucrt.lib
      vcruntime.lib
      msvcrt.lib
  )
  # Eliminar conflictos de librerías por defecto
  target_link_options(${PROJECT_NAME} PRIVATE 
    "/NODEFAULTLIB:LIBCMT" 
    "/NODEFAULTLIB:LIBCMTD"
    )
endif()





# Configuración específica para GCC/MinGW
if (NOT MSVC)
  target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
  # Vinculación de hilos, C++ (STL) y GCC (Soporte de compilador) para GCC/MinGW
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE 
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
