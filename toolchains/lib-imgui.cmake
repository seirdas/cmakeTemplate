
# ------------------------------
# LIBRERÍA GRÁFICA CON GLFW + IMGUI + OTRAS DEPENDENCIAS
# Usa OpenGL
# Genera la librería imgui_lib
# ------------------------------

include(FetchContent)

# GLFW (Ventanas) ___________________________
message(STATUS "Fetching GLFW library...")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # No construir ejemplos
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)    # No construir tests
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)    # No construir documentación
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)    # No instalar
if (MSVC)
  set(GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
endif()

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/glfw_src/CMakeLists.txt")
  message(STATUS "Using local GLFW source")
  set(FETCHCONTENT_SOURCE_DIR_GLFW
      "${EXTERNAL_LIB_PATH}/glfw_src"
      CACHE PATH "" FORCE)
endif()

# declara el recurso externo que CMake descargará
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW    TRUE         # habilita --depth 1
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/glfw_src"
  EXCLUDE_FROM_ALL TRUE
)
# Hace disponible el recurso
FetchContent_MakeAvailable(glfw)
find_package(OpenGL REQUIRED)   # Necesita librería de OpenGL (GLFW la usa para renderizar)


# IMGUI (Interfaz) ___________________________
message(STATUS "Fetching ImGui library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/imgui_src/.github")
  message(STATUS "Using local ImGui source")
  set(FETCHCONTENT_SOURCE_DIR_IMGUI
      "${EXTERNAL_LIB_PATH}/imgui_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.5-docking      # o la versión que necesites
  GIT_SHALLOW    TRUE          # habilita --depth 1
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/imgui_src"
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_DIR)
file(GLOB IMGUI_SOURCES 
  "${IMGUI_DIR}/*.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp" 
)


# Crear librería estática con ImGui + GLFW ____________________________
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_link_libraries(imgui_lib PUBLIC 
  glfw                # GLFW
  OpenGL::GL          # OpenGL                
  $<$<PLATFORM_ID:Windows>:
    user32    # Ventanas y controles básicos de Windows
    gdi32     # Gráficos básicos de Windows
    shell32   # Funciones de shell de Windows
    imm32     # Soporte de IME (Input Method Editor) para entrada de texto avanzada
  >  # Librerías de sistema en Windows
)
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)   # Directorios propios
# target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)      # Usar GLAD como cargador de OpenGL
