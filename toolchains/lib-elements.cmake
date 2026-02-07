# ============================================================
# cycfi dependencies: q, infra, elements
# ============================================================

include(FetchContent)

# -------------------------------
# cycfi/q (header-only)
# -------------------------------

message(STATUS "Fetching cycfi/q library...")

if (EXISTS "${EXTERNAL_LIB_PATH}/q_src/.git")
  message(STATUS "Using local cycfi/q source")
  set(FETCHCONTENT_SOURCE_DIR_Q
      "${EXTERNAL_LIB_PATH}/q_src"
      CACHE PATH "" FORCE)
endif()

# Declarar dependencia externa
FetchContent_Declare(
  q
  GIT_REPOSITORY https://github.com/cycfi/q.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/q_src"
  EXCLUDE_FROM_ALL TRUE
)
# Hacerla disponible
FetchContent_MakeAvailable(q)

add_library(q_lib INTERFACE)

target_include_directories(q_lib INTERFACE
  ${q_SOURCE_DIR}/include
)

target_compile_features(q_lib INTERFACE cxx_std_17)


# -------------------------------
# cycfi/infra
# -------------------------------

message(STATUS "Fetching cycfi/infra library...")

if (EXISTS "${EXTERNAL_LIB_PATH}/infra_src/.git")
  message(STATUS "Using local cycfi/infra source")
  set(FETCHCONTENT_SOURCE_DIR_INFRA
      "${EXTERNAL_LIB_PATH}/infra_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  infra
  GIT_REPOSITORY https://github.com/cycfi/infra.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/infra_src"
  EXCLUDE_FROM_ALL TRUE
)

FetchContent_MakeAvailable(infra)


# -------------------------------
# OpenGL (común)
# -------------------------------

find_package(OpenGL REQUIRED)


# ============================================================
# Linux/Linux dependencies
# ============================================================

if (UNIX AND NOT APPLE)

  message(STATUS "Configuring Linux dependencies for Elements...")

  find_package(X11 REQUIRED)
  find_package(Freetype REQUIRED)
  find_package(Fontconfig REQUIRED)
  find_package(PkgConfig REQUIRED)

  pkg_check_modules(WAYLAND wayland-client)

elseif(WIN32)

  message(STATUS "Configuring Windows dependencies for Elements...")

  find_package(OpenGL REQUIRED)

  # GDI / User32 / etc
  set(WIN32_LIBS
    user32
    gdi32
    shell32
    ole32
    comdlg32
  )

endif()


# -------------------------------
# cycfi/elements
# -------------------------------

message(STATUS "Fetching cycfi/elements library...")

# Desactivar extras
set(ELEMENTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ELEMENTS_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

if (EXISTS "${EXTERNAL_LIB_PATH}/elements_src/.git")
  message(STATUS "Using local cycfi/elements source")
  set(FETCHCONTENT_SOURCE_DIR_ELEMENTS
      "${EXTERNAL_LIB_PATH}/elements_src"
      CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
  elements
  GIT_REPOSITORY https://github.com/cycfi/elements.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  SOURCE_DIR     "${EXTERNAL_LIB_PATH}/elements_src"
  EXCLUDE_FROM_ALL TRUE
)

FetchContent_MakeAvailable(elements)


# -------------------------------
# Wrapper target
# -------------------------------

add_library(elements_lib INTERFACE)


if (UNIX AND NOT APPLE)
  message(STATUS "Finding Linux dependencies for Elements...")

  find_package(X11 REQUIRED)
  find_package(Freetype REQUIRED)
  find_package(Fontconfig REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND REQUIRED wayland-client)
endif()


target_link_libraries(elements_lib PUBLIC
  elements
  q_lib
  infra
  OpenGL::GL
  
  $<$<PLATFORM_ID:Windows>:  # Librerías de sistema en Windows
    user32      # Ventanas y eventos
    gdi32       # Gráficos básicos
    shell32     # Shell API
    ole32       # COM / OLE
    comdlg32    # Diálogos comunes
    opengl32    # OpenGL en Windows
  >
  $<$<PLATFORM_ID:Linux>:   # Librerías de sistema en Linux
    X11::X11                 # Librerías de ventanas X11
    ${WAYLAND_LIBRARIES}     # Librerías de ventanas Wayland
    Freetype::Freetype       # Render de fuentes
    Fontconfig::Fontconfig   # Configuración de fuentes
    pthread                  # Threads
    dl                       # Dynamic loader
    m                        # Math
  >
)

target_include_directories(elements_lib PUBLIC
  $<$<PLATFORM_ID:Linux>:${WAYLAND_INCLUDE_DIRS}>
)
target_compile_features(elements_lib INTERFACE cxx_std_17)
