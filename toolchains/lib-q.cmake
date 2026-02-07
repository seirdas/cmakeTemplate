# -------------------------------
# Librería cycfi/q (Header-only, Functional)
# -------------------------------

message(STATUS "Fetching cycfi/q library...")

# Usa la librería ya descargada en external/ si existe
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

# Crear librería INTERFACE (header-only)
add_library(q_lib INTERFACE)

target_include_directories(q_lib INTERFACE
  ${q_SOURCE_DIR}/include
)

target_compile_features(q_lib INTERFACE cxx_std_17)
