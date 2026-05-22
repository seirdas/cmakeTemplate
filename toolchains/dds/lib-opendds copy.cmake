# ==============================
# toolchains/net/lib-opendds.cmake
# ==============================
# Integración de OpenDDS mediante FetchContent.
# Fuentes 100% persistentes en _external/ → funciona sin internet tras la primera descarga.
#
# ESTRUCTURA EN DISCO (nada va al directorio de build salvo los .o/.a):
#   _external/opendds_src/     ← fuentes OpenDDS            (NO borrar)
#   _external/acetao_src/      ← fuentes ACE/TAO            (NO borrar)
#   IDL/generated_opendds/     ← .cpp/.h generados desde IDL/
#
# FLUJO DE USO:
#   1ª vez (internet disponible):
#     cmake -B _build/xxx              → descarga OpenDDS+ACE/TAO a _external/
#     cmake --build _build/xxx         → compila todo
#
#   Siguientes veces (sin internet):
#     cmake -B _build/xxx              → usa _external/ directamente, cero red
#     cmake --build _build/xxx         → recompila solo lo que haya cambiado
#
# NOTA NINJA + ACE/TAO:
#   El CMake de OpenDDS compila ACE/TAO usando GNU Make o Visual Studio internamente
#   (MPC no soporta Ninja). El resto del proyecto sí puede usar Ninja con normalidad.
#   Ref: https://opendds.readthedocs.io/en/latest/devguide/building/cmake.html
# ==============================

include_guard(GLOBAL)

# ──────────────────────────────────────────────────────────────────────────────
# 0. Requisito mínimo de CMake
#    SOURCE_DIR en FetchContent_Declare requiere >= 3.24
#    OpenDDS CMake build requiere >= 3.23 (añadido en OpenDDS 3.26)
# ──────────────────────────────────────────────────────────────────────────────
if(CMAKE_VERSION VERSION_LESS "3.24")
    message(FATAL_ERROR
        "[OpenDDS] Se requiere CMake >= 3.24 para SOURCE_DIR persistente en FetchContent.\n"
        "Versión actual: ${CMAKE_VERSION}"
    )
endif()

# ──────────────────────────────────────────────────────────────────────────────
# 1. Versiones configurables
#    Para encontrar el tag ACE/TAO correcto para tu versión de OpenDDS, consulta:
#    https://github.com/OpenDDS/OpenDDS/blob/DDS-X.Y.Z/cmake/FetchACETAO.cmake
# ──────────────────────────────────────────────────────────────────────────────
set(OPENDDS_VERSION "3.28.1"         CACHE STRING "Versión de OpenDDS")
set(ACETAO_GIT_TAG  "ACE+TAO-7.1.1"  CACHE STRING
    "Tag git de ACE/TAO compatible con OPENDDS_VERSION. \
Verifica en cmake/FetchACETAO.cmake dentro del repo de OpenDDS.")

# ──────────────────────────────────────────────────────────────────────────────
# 2. Rutas persistentes — siguen la convención del proyecto (EXTERNAL_LIB_PATH)
# ──────────────────────────────────────────────────────────────────────────────
set(OPENDDS_SRC_DIR  "${EXTERNAL_LIB_PATH}/opendds_src" CACHE PATH "Fuentes de OpenDDS")
set(ACETAO_SRC_DIR   "${EXTERNAL_LIB_PATH}/acetao_src"  CACHE PATH "Fuentes de ACE/TAO")

# Directorio de los IDL fuente del proyecto y de los archivos que se generarán
set(OPENDDS_IDL_SOURCE_DIR    "${CMAKE_SOURCE_DIR}/IDL")
set(OPENDDS_IDL_GENERATED_DIR "${CMAKE_SOURCE_DIR}/IDL/generated_opendds")

# ──────────────────────────────────────────────────────────────────────────────
# 3. Helpers internos
# ──────────────────────────────────────────────────────────────────────────────

# TRUE si el directorio existe y contiene al menos un elemento
function(_odds_is_populated DIR OUT_VAR)
    if(EXISTS "${DIR}" AND IS_DIRECTORY "${DIR}")
        file(GLOB _items LIST_DIRECTORIES true "${DIR}/*")
        if(_items)
            set(${OUT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
    endif()
    set(${OUT_VAR} FALSE PARENT_SCOPE)
endfunction()

# TRUE si GitHub es accesible (timeout corto, sin bloquear)
function(_odds_has_internet OUT_VAR)
    set(_tmp "${CMAKE_BINARY_DIR}/_odds_net_probe.tmp")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/OpenDDS/OpenDDS/master/VERSION.txt"
        "${_tmp}" TIMEOUT 6 STATUS _st
    )
    list(GET _st 0 _code)
    file(REMOVE "${_tmp}")
    if(_code EQUAL 0)
        set(${OUT_VAR} TRUE  PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# FATAL_ERROR con mensaje útil si no hay fuentes ni red
macro(_odds_require_internet_for NAME DIR)
    _odds_has_internet(_odds_online)
    if(NOT _odds_online)
        message(FATAL_ERROR
            "\n"
            "══════════════════════════════════════════════════════════════════\n"
            "  ERROR: '${NAME}' no está en _external/ y no hay conexión a internet.\n"
            "\n"
            "  Directorio esperado:\n"
            "    ${DIR}\n"
            "\n"
            "  Solución:\n"
            "    1. Ejecuta cmake una vez en una máquina con internet.\n"
            "       Las fuentes se descargarán a _external/ de forma permanente.\n"
            "    2. Copia el proyecto completo (incluyendo _external/) a la\n"
            "       máquina sin internet y vuelve a ejecutar cmake con normalidad.\n"
            "══════════════════════════════════════════════════════════════════\n"
        )
    endif()
endmacro()

# ──────────────────────────────────────────────────────────────────────────────
# 4. Verificar disponibilidad de fuentes / conectividad
# ──────────────────────────────────────────────────────────────────────────────
_odds_is_populated("${OPENDDS_SRC_DIR}" _opendds_present)
_odds_is_populated("${ACETAO_SRC_DIR}"  _acetao_present)

if(_opendds_present)
    message(STATUS "[OpenDDS] OpenDDS → fuentes locales en ${OPENDDS_SRC_DIR}")
else()
    message(STATUS "[OpenDDS] OpenDDS → no encontrado en _external/, verificando red...")
    _odds_require_internet_for("OpenDDS" "${OPENDDS_SRC_DIR}")
    message(STATUS "[OpenDDS] OpenDDS → descargando v${OPENDDS_VERSION}...")
endif()

if(_acetao_present)
    message(STATUS "[OpenDDS] ACE/TAO → fuentes locales en ${ACETAO_SRC_DIR}")
else()
    message(STATUS "[OpenDDS] ACE/TAO → no encontrado en _external/, verificando red...")
    _odds_require_internet_for("ACE/TAO" "${ACETAO_SRC_DIR}")
    message(STATUS "[OpenDDS] ACE/TAO → descargando tag ${ACETAO_GIT_TAG}...")
endif()

# ──────────────────────────────────────────────────────────────────────────────
# 5. FetchContent de ACE/TAO
#    Se descarga ANTES que OpenDDS para poder pasarle OPENDDS_ACE_TAO_SRC.
#    Repositorio oficial: https://github.com/DOCGroup/ACE_TAO
#
#    Nota: FetchContent_Populate (en vez de MakeAvailable) porque ACE/TAO
#    no tiene un CMakeLists.txt raíz que integrar en el proyecto — OpenDDS
#    lo compila internamente mediante MPC + GNU Make / MSBuild.
# ──────────────────────────────────────────────────────────────────────────────
include(FetchContent)

if(_acetao_present)
    # Fuentes ya en disco → evitar cualquier acceso a red
    set(FETCHCONTENT_SOURCE_DIR_ACE_TAO "${ACETAO_SRC_DIR}" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    ace_tao
    GIT_REPOSITORY "https://github.com/DOCGroup/ACE_TAO.git"
    GIT_TAG        "${ACETAO_GIT_TAG}"
    GIT_SHALLOW    FALSE        # FALSE: repo completo necesario para uso offline
    GIT_PROGRESS   TRUE
    SOURCE_DIR     "${ACETAO_SRC_DIR}"   # ← persistente en _external/acetao_src/
)

FetchContent_Populate(ace_tao)
message(STATUS "[OpenDDS] ACE/TAO source listo en: ${ACETAO_SRC_DIR}")

# ──────────────────────────────────────────────────────────────────────────────
# 6. Opciones de OpenDDS — deben estar en el cache ANTES de FetchContent_MakeAvailable
#
#    IMPORTANTE: En FetchContent los sub-proyectos comparten el cache CMake del
#    padre. Las opciones se pasan con set(VAR val CACHE TYPE "" FORCE), NO con
#    CMAKE_CACHE_ARGS en FetchContent_Declare (eso es solo para ExternalProject).
#    Ref: https://cmake.org/cmake/help/latest/module/FetchContent.html
# ──────────────────────────────────────────────────────────────────────────────

# Deshabilitar tests y ejemplos (reducen drásticamente el tiempo de build)
set(OPENDDS_BUILD_TESTS    OFF CACHE BOOL "No compilar tests de OpenDDS"    FORCE)
set(OPENDDS_BUILD_EXAMPLES OFF CACHE BOOL "No compilar ejemplos de OpenDDS" FORCE)

# Apuntar a las fuentes ACE/TAO que acabamos de descargar/restaurar.
# Con OPENDDS_ACE_TAO_SRC definido, OpenDDS NO descarga ACE/TAO él mismo.
# Ref: https://opendds.readthedocs.io/en/latest/devguide/building/cmake.html#opendds-ace-tao-src
set(OPENDDS_ACE_TAO_SRC "${ACETAO_SRC_DIR}" CACHE PATH
    "Directorio con fuentes de ACE/TAO para OpenDDS" FORCE)

# Características opcionales (ajusta según necesidades del proyecto)
set(OPENDDS_SECURITY          OFF CACHE BOOL "" FORCE) # ON requiere OpenSSL + Xerces
set(OPENDDS_SAFETY_PROFILE    OFF CACHE BOOL "" FORCE)
set(OPENDDS_NO_BUILT_IN_TOPICS OFF CACHE BOOL "" FORCE)

# Unity Builds: varios .cpp compilados por proceso → build mucho más rápido.
# Si aparecen errores de redefinición, desactiva con -DCMAKE_UNITY_BUILD=FALSE.
# Ref: https://opendds.readthedocs.io/en/latest/devguide/building/cmake.html#speeding-up-the-build
set(CMAKE_UNITY_BUILD ON CACHE BOOL "Unity builds (acelera la compilación de OpenDDS)" FORCE)

# Versión de ACE/TAO a usar (ace7tao3 = ACE 7 / TAO 3, es el default)
# Solo cambiar si usas una versión distinta de OpenDDS que requiera otra rama.
# set(OPENDDS_ACE_TAO_KIND "ace7tao3" CACHE STRING "" FORCE)

# ──────────────────────────────────────────────────────────────────────────────
# 7. FetchContent de OpenDDS
# ──────────────────────────────────────────────────────────────────────────────
if(_opendds_present)
    set(FETCHCONTENT_SOURCE_DIR_OPENDDS "${OPENDDS_SRC_DIR}" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    opendds
    GIT_REPOSITORY "https://github.com/OpenDDS/OpenDDS.git"
    GIT_TAG        "DDS-${OPENDDS_VERSION}"
    GIT_SHALLOW    FALSE        # FALSE: repo completo necesario para uso offline
    GIT_PROGRESS   TRUE
    SOURCE_DIR     "${OPENDDS_SRC_DIR}"   # ← persistente en _external/opendds_src/
)

FetchContent_MakeAvailable(opendds)

# Guardar rutas para los pasos de generación IDL
set(OPENDDS_SOURCE_DIR "${opendds_SOURCE_DIR}" CACHE INTERNAL "OpenDDS source dir")
set(OPENDDS_BINARY_DIR "${opendds_BINARY_DIR}" CACHE INTERNAL "OpenDDS binary dir")

message(STATUS "[OpenDDS] Source: ${OPENDDS_SOURCE_DIR}")
message(STATUS "[OpenDDS] Binary: ${OPENDDS_BINARY_DIR}")

# ──────────────────────────────────────────────────────────────────────────────
# 8. Verificar targets CMake de OpenDDS
# ──────────────────────────────────────────────────────────────────────────────
foreach(_t OpenDDS::Dcps OpenDDS::Rtps OpenDDS::Rtps_Udp)
    if(NOT TARGET ${_t})
        message(FATAL_ERROR
            "[OpenDDS] Target '${_t}' no disponible tras FetchContent_MakeAvailable.\n"
            "Revisa el log de CMake para errores en la configuración de OpenDDS."
        )
    endif()
endforeach()
message(STATUS "[OpenDDS] Targets disponibles: OpenDDS::Dcps  OpenDDS::Rtps  OpenDDS::Rtps_Udp ✓")

# ──────────────────────────────────────────────────────────────────────────────
# 9. Localizar compiladores IDL del build local
#    opendds_idl y tao_idl se construyen como parte de OpenDDS.
#    NO_DEFAULT_PATH: solo se busca en el build local, nunca en el sistema,
#    para evitar versiones incompatibles instaladas globalmente.
# ──────────────────────────────────────────────────────────────────────────────
find_program(OPENDDS_IDL_BIN
    NAMES opendds_idl
    HINTS
        "${OPENDDS_BINARY_DIR}/bin"
        "${CMAKE_BINARY_DIR}/_deps/opendds-build/bin"
    NO_DEFAULT_PATH
    DOC "Compilador opendds_idl del build local"
)

find_program(TAO_IDL_BIN
    NAMES tao_idl
    HINTS
        "${OPENDDS_BINARY_DIR}/bin"
        "${CMAKE_BINARY_DIR}/_deps/opendds-build/bin"
        "${ACETAO_SRC_DIR}/bin"
    NO_DEFAULT_PATH
    DOC "Compilador tao_idl del build local de ACE/TAO"
)

# Los compiladores pueden no existir aún en configure-time (se compilan en build-time).
# Calculamos la ruta esperada para usarla como COMMAND en los custom commands;
# los DEPENDS garantizan que existan antes de que se invoquen.
if(OPENDDS_IDL_BIN)
    message(STATUS "[OpenDDS] opendds_idl : ${OPENDDS_IDL_BIN}")
else()
    set(OPENDDS_IDL_BIN "${OPENDDS_BINARY_DIR}/bin/opendds_idl")
    message(STATUS "[OpenDDS] opendds_idl : pendiente de compilar → ${OPENDDS_IDL_BIN}")
endif()

if(TAO_IDL_BIN)
    message(STATUS "[OpenDDS] tao_idl     : ${TAO_IDL_BIN}")
else()
    set(TAO_IDL_BIN "${OPENDDS_BINARY_DIR}/bin/tao_idl")
    message(STATUS "[OpenDDS] tao_idl     : pendiente de compilar → ${TAO_IDL_BIN}")
endif()

# ──────────────────────────────────────────────────────────────────────────────
# 10. Recopilar .idl del proyecto
#     Busca en IDL/ de forma recursiva.
#     Excluye generated_opendds/ (esos son outputs, no inputs).
# ──────────────────────────────────────────────────────────────────────────────
file(GLOB_RECURSE OPENDDS_IDL_FILES
    CONFIGURE_DEPENDS
    "${OPENDDS_IDL_SOURCE_DIR}/*.idl"
)
list(FILTER OPENDDS_IDL_FILES EXCLUDE REGEX ".*/generated_opendds/.*")

if(NOT OPENDDS_IDL_FILES)
    message(WARNING "[OpenDDS] No se encontraron archivos .idl en ${OPENDDS_IDL_SOURCE_DIR}")
else()
    message(STATUS "[OpenDDS] IDL a procesar:")
    foreach(_f ${OPENDDS_IDL_FILES})
        cmake_path(GET _f FILENAME _fn)
        message(STATUS "           ├─ ${_fn}")
    endforeach()
    message(STATUS "           └─ Output → ${OPENDDS_IDL_GENERATED_DIR}")
endif()

file(MAKE_DIRECTORY "${OPENDDS_IDL_GENERATED_DIR}")

# ──────────────────────────────────────────────────────────────────────────────
# 11. Generación de código C++ desde los IDL
#
#  Por cada MyTopic.idl se generan (en IDL/generated_opendds/):
#
#  Paso A — opendds_idl sobre el .idl original:
#    MyTopicTypeSupport.cpp / .h / .idl
#
#  Paso B — tao_idl sobre el .idl original:
#    MyTopicC.cpp / .h   (client stubs)
#    MyTopicS.cpp / .h   (server skeletons)
#
#  Paso C — tao_idl sobre el MyTopicTypeSupport.idl generado en A:
#    MyTopicTypeSupportC.cpp / .h
#    MyTopicTypeSupportS.cpp / .h
# ──────────────────────────────────────────────────────────────────────────────
set(_odds_all_sources "")
set(_odds_all_headers "")

# Flags -I compartidos por todos los pasos
set(_idl_inc
    "-I${OPENDDS_IDL_SOURCE_DIR}"
    "-I${OPENDDS_IDL_GENERATED_DIR}"
    "-I${OPENDDS_SOURCE_DIR}"    # necesario para los headers internos de OpenDDS
)

foreach(_idl ${OPENDDS_IDL_FILES})
    cmake_path(GET _idl STEM        _name)
    cmake_path(GET _idl PARENT_PATH _idl_dir)

    # ── A) opendds_idl → TypeSupport.{cpp,h,idl} ─────────────────────────
    set(_TS_cpp "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupport.cpp")
    set(_TS_h   "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupport.h")
    set(_TS_idl "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupport.idl")

    add_custom_command(
        OUTPUT  "${_TS_cpp}" "${_TS_h}" "${_TS_idl}"
        COMMAND "${OPENDDS_IDL_BIN}"
                ${_idl_inc}
                "-I${_idl_dir}"
                -o "${OPENDDS_IDL_GENERATED_DIR}"
                "${_idl}"
        DEPENDS
                "${_idl}"
                opendds          # garantiza que opendds_idl esté compilado primero
        COMMENT "[opendds_idl] ${_name}.idl → TypeSupport"
        VERBATIM
    )

    # ── B) tao_idl sobre el .idl original → *C.* y *S.* ─────────────────
    set(_C_cpp  "${OPENDDS_IDL_GENERATED_DIR}/${_name}C.cpp")
    set(_C_h    "${OPENDDS_IDL_GENERATED_DIR}/${_name}C.h")
    set(_S_cpp  "${OPENDDS_IDL_GENERATED_DIR}/${_name}S.cpp")
    set(_S_h    "${OPENDDS_IDL_GENERATED_DIR}/${_name}S.h")

    add_custom_command(
        OUTPUT  "${_C_cpp}" "${_C_h}" "${_S_cpp}" "${_S_h}"
        COMMAND "${TAO_IDL_BIN}"
                ${_idl_inc}
                "-I${_idl_dir}"
                -o "${OPENDDS_IDL_GENERATED_DIR}"
                -Wb,pre_include=ace/pre.h
                -Wb,post_include=ace/post.h
                "${_idl}"
        DEPENDS
                "${_idl}"
                opendds
        COMMENT "[tao_idl] ${_name}.idl → C/S stubs"
        VERBATIM
    )

    # ── C) tao_idl sobre TypeSupport.idl (output del paso A) → TypeSupportC/S.*
    set(_TSC_cpp "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupportC.cpp")
    set(_TSC_h   "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupportC.h")
    set(_TSS_cpp "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupportS.cpp")
    set(_TSS_h   "${OPENDDS_IDL_GENERATED_DIR}/${_name}TypeSupportS.h")

    add_custom_command(
        OUTPUT  "${_TSC_cpp}" "${_TSC_h}" "${_TSS_cpp}" "${_TSS_h}"
        COMMAND "${TAO_IDL_BIN}"
                ${_idl_inc}
                "-I${_idl_dir}"
                -o "${OPENDDS_IDL_GENERATED_DIR}"
                -Wb,pre_include=ace/pre.h
                -Wb,post_include=ace/post.h
                "${_TS_idl}"
        DEPENDS
                "${_TS_idl}"   # depende del .idl generado en el paso A
                opendds
        COMMENT "[tao_idl] ${_name}TypeSupport.idl → TypeSupportC/S"
        VERBATIM
    )

    list(APPEND _odds_all_sources
        "${_C_cpp}"   "${_S_cpp}"
        "${_TS_cpp}"
        "${_TSC_cpp}" "${_TSS_cpp}"
    )
    list(APPEND _odds_all_headers
        "${_C_h}"   "${_S_h}"
        "${_TS_h}"
        "${_TSC_h}" "${_TSS_h}"
    )

endforeach()

# ──────────────────────────────────────────────────────────────────────────────
# 12. Librería estática con el código IDL generado
#     Nombre del target: opendds_idl_generated
#
#     El CMakeLists.txt principal enlaza con:
#       $<$<BOOL:${USE_OPENDDS}>:opendds_idl_generated>
# ──────────────────────────────────────────────────────────────────────────────
if(_odds_all_sources)

    add_library(opendds_idl_generated STATIC ${_odds_all_sources})

    # OPENDDS_TARGET_SOURCES: macro oficial de OpenDDS que registra los IDL
    # en el target y genera el TypeSupport glue adicional que necesita internamente.
    OPENDDS_TARGET_SOURCES(opendds_idl_generated
        PUBLIC
            ${OPENDDS_IDL_FILES}
        OPENDDS_IDL_OPTIONS
            ${_idl_inc}
        TAO_IDL_OPTIONS
            ${_idl_inc}
            -Wb,pre_include=ace/pre.h
            -Wb,post_include=ace/post.h
    )

    target_include_directories(opendds_idl_generated
        PUBLIC
            "${OPENDDS_IDL_GENERATED_DIR}"
            "${OPENDDS_IDL_SOURCE_DIR}"
    )

    target_link_libraries(opendds_idl_generated
        PUBLIC
            OpenDDS::Dcps
            OpenDDS::Rtps
            OpenDDS::Rtps_Udp
    )

    message(STATUS "[OpenDDS] Target 'opendds_idl_generated' configurado ✓")

else()
    # Sin IDL: target vacío para que el enlace no falle
    add_library(opendds_idl_generated INTERFACE)
    target_link_libraries(opendds_idl_generated
        INTERFACE
            OpenDDS::Dcps
            OpenDDS::Rtps
            OpenDDS::Rtps_Udp
    )
    message(WARNING "[OpenDDS] Sin archivos IDL que procesar → target INTERFACE vacío creado.")
endif()

# Variables exportadas para uso desde otros módulos
set(OPENDDS_GENERATED_SOURCES "${_odds_all_sources}" CACHE INTERNAL "")
set(OPENDDS_GENERATED_HEADERS "${_odds_all_headers}" CACHE INTERNAL "")