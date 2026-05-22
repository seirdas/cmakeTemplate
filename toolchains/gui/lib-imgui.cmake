
# ------------------------------
# LIBRERÍA GRÁFICA CON GLFW + IMGUI + OTRAS DEPENDENCIAS
# Usa OpenGL
# Genera la librería imgui_lib
# ------------------------------

include(FetchContent)

# Para Linux necesita mínimo X11
if(LINUX)
    find_package(X11 REQUIRED)
endif()

# Build en static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# GLFW (Ventanas) ___________________________
message(STATUS "[ImGui] Fetching GLFW library...")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # No construir ejemplos
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)    # No construir tests
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)    # No construir documentación
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)    # No instalar
if (MSVC)
  set(GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
endif()

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/glfw_src/.git")
  message(STATUS "[ImGui] Source of 'GLFW' found locally at: '${EXTERNAL_LIB_PATH}/glfw_src'")
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
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)
# Hace disponible el recurso
FetchContent_MakeAvailable(glfw)

# Seleccionar OpenGL Vendor-Neutral Dispatcher (Redirección de llamadas a GPU)
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)   # Necesita librería de OpenGL (GLFW la usa para renderizar)


# IMGUI (Interfaz) ___________________________
message(STATUS "[ImGui] Fetching ImGui library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/imgui_src/.git")
  message(STATUS "[ImGui] Source of 'ImGui' found locally at: '${EXTERNAL_LIB_PATH}/imgui_src'")
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
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_DIR)
file(GLOB IMGUI_SOURCES 
  "${IMGUI_DIR}/*.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp" 
)

# IMPLOT (Gráficos) ___________________________
message(STATUS "[ImGui] Fetching Implot library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/implot_src/.git")
  message(STATUS "[ImGui] Source of 'ImPlot' found locally at: '${EXTERNAL_LIB_PATH}/implot_src'")
  set(FETCHCONTENT_SOURCE_DIR_IMPLOT
      "${EXTERNAL_LIB_PATH}/implot_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.17   # o la versión que necesites
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/implot_src"
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(implot)
FetchContent_GetProperties(implot SOURCE_DIR IMPLOT_DIR)

file(GLOB IMPLOT_SOURCES
  "${IMPLOT_DIR}/*.cpp"
)

# IMGUI-KNOBS (Controles circulares) ___________________________
message(STATUS "[ImGui] Fetching ImGui-Knobs library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/imgui_knobs_src/README.md")
  message(STATUS "[ImGui] Source of 'imgui_knobs' found locally at: '${EXTERNAL_LIB_PATH}/imgui_knobs_src'")
  set(FETCHCONTENT_SOURCE_DIR_IMGUI_KNOBS
      "${EXTERNAL_LIB_PATH}/imgui_knobs_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  imgui_knobs
  GIT_REPOSITORY https://github.com/altschuler/imgui-knobs.git
  GIT_TAG main  # o la versión que necesites
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/imgui_knobs_src"
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)

FetchContent_MakeAvailable(imgui_knobs)
FetchContent_GetProperties(imgui_knobs SOURCE_DIR IMGUI_KNOBS_DIR)

# imgui-knobs suele ser un header (.h) y un source (.cpp)
file(GLOB IMGUI_KNOBS_SOURCES
  "${IMGUI_KNOBS_DIR}/imgui-knobs.cpp"
)

# IMSPINNER (Indicadores de carga) ___________________________
message(STATUS "[ImGui] Fetching ImSpinner library...")

# Usa la librería ya descargada en external/ si existe
if (EXISTS "${EXTERNAL_LIB_PATH}/imspinner_src/.git")
  message(STATUS "[ImGui] Source of 'imspinner' found locally at: '${EXTERNAL_LIB_PATH}/imspinner_src'")
  set(FETCHCONTENT_SOURCE_DIR_IMSPINNER
      "${EXTERNAL_LIB_PATH}/imspinner_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  imspinner
  GIT_REPOSITORY https://github.com/dalerank/imspinner
  GIT_TAG master
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/imspinner_src"
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
  EXCLUDE_FROM_ALL TRUE
)
FetchContent_MakeAvailable(imspinner)

# Obtener rutas para añadirlo a la librería imgui
FetchContent_GetProperties(imspinner SOURCE_DIR IMSPINNER_DIR)

# Crear librería estática con GLFW + ImGui + dependencias _________________

add_library(imgui_lib STATIC 
  ${IMGUI_SOURCES}
  ${IMPLOT_SOURCES}
  ${IMGUI_KNOBS_SOURCES}
)

target_link_libraries(imgui_lib PUBLIC 
  glfw                # GLFW
  OpenGL::GL          # OpenGL 

  $<$<PLATFORM_ID:Windows>:
    user32    # Ventanas y controles básicos de Windows
    gdi32     # Gráficos básicos de Windows
    shell32   # Funciones de shell de Windows
    imm32     # Soporte de IME (Input Method Editor) para entrada de texto avanzada
  >
  $<$<PLATFORM_ID:Linux>:
    X11           # Librería base de X11
    ${X11_LIBRARIES}
    Xcursor       # Gestión de cursores
    Xinerama      # Soporte multimonitor
    Xrandr        # Resolución de pantalla
    Xi            # Input avanzado
    dl            # Cargador dinámico (necesario para drivers)
    pthread       # Hilos del sistema
  >
)

target_include_directories(imgui_lib SYSTEM PUBLIC 
  ${IMGUI_DIR}            # imgui 
  ${IMGUI_DIR}/backends   # imgui backends
  ${IMPLOT_DIR}           # implot
  ${IMGUI_KNOBS_DIR}      # imgui knobs
  ${IMSPINNER_DIR}        # imgui spinners
  $<$<PLATFORM_ID:Linux>:${X11_INCLUDE_DIR}> # Librerías X11 (Linux)
) 

target_compile_definitions(imgui_lib PUBLIC
    $<$<PLATFORM_ID:Windows>:
      GLFW_EXPOSE_NATIVE_WIN32  # Exponer funciones nativas de Windows en GLFW
    >
    $<$<PLATFORM_ID:Linux>:
      GLFW_EXPOSE_NATIVE_X11      # Permite a ImGui usar funciones de X11
      GLFW_EXPOSE_NATIVE_WAYLAND  # Permite a ImGui usar funciones de Wayland
    >

    # IMGUI_IMPL_OPENGL_LOADER_GLAD  # Usar GLAD como cargador de OpenGL
)

if(LINUX)
    # Forzamos la inclusión de Xatom.h para que XA_ATOM esté definido
    target_compile_options(imgui_lib PRIVATE 
      $<$<COMPILE_LANGUAGE:CXX>:-include X11/Xatom.h>
    )
endif()

# Omitir warnings de la librería
target_compile_options(imgui_lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W0            # Nivel de advertencia 0 (silencio total)
        /wd4244        # double a float
        /wd4305        # truncamiento de constantes
        /wd4267        # size_t a int
        /external:W0   # (CMake 3.22+) Silencia cabeceras externas
    >
    
    # --- Configuración para GCC / Clang / MinGW ---
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -w             # Suprime todos los warnings
        -Wno-conversion
        -Wno-sign-compare
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-shadow
    >
)
