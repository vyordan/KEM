cmake_minimum_required(VERSION 3.20)
project(kem VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ── LLVM ──────────────────────────────────────────────────────────────────────
#
# KEM puede linkear LLVM de dos formas distintas, elegidas con la opción
# KEM_STATIC_LLVM (o detectado automáticamente si se pasa LLVM_STATIC_ROOT):
#
#   1. DINÁMICO (default, para desarrollo en tu propia máquina):
#      Usa el LLVM que ya tenés instalado vía pacman (libLLVM.so).
#      El binario `kem` resultante depende de esa .so en runtime —
#      cualquiera que lo ejecute necesita tener LLVM instalado.
#      Esto es lo que usábamos hasta ahora.
#
#   2. ESTÁTICO (para generar el binario DISTRIBUIBLE a estudiantes):
#      Usa una instalación de LLVM compilada con BUILD_SHARED_LIBS=OFF
#      (ver guía de compilación). El binario `kem` resultante no depende
#      de ninguna .so de LLVM — el código de LLVM queda embebido dentro
#      del ejecutable. Esto es lo que hay que usar para el binario que
#      se sube a GitHub Releases / se distribuye a estudiantes.
#
# Uso:
#   # Build normal (dinámico, para desarrollo):
#   cmake -B build -G Ninja
#
#   # Build estático (para distribuir):
#   cmake -B build-static -G Ninja \
#       -DLLVM_STATIC_ROOT=$HOME/llvm-static-install \
#       -DCMAKE_BUILD_TYPE=Release
#
option(KEM_STATIC_LLVM "Linkear LLVM estáticamente (para distribución)" OFF)
set(LLVM_STATIC_ROOT "" CACHE PATH
    "Ruta a una instalación de LLVM compilada estáticamente (ninja install). Si se define, activa KEM_STATIC_LLVM automáticamente.")

if(LLVM_STATIC_ROOT)
    set(KEM_STATIC_LLVM ON)
endif()

if(KEM_STATIC_LLVM)
    # ── Modo estático ───────────────────────────────────────────────────────
    if(NOT LLVM_STATIC_ROOT)
        message(FATAL_ERROR
            "KEM_STATIC_LLVM está activado pero no se definió LLVM_STATIC_ROOT.\n"
            "Pasá la ruta de tu instalación estática, ej:\n"
            "  cmake -B build-static -DLLVM_STATIC_ROOT=$HOME/llvm-static-install")
    endif()

    set(LLVM_CONFIG_EXECUTABLE "${LLVM_STATIC_ROOT}/bin/llvm-config")
    if(NOT EXISTS "${LLVM_CONFIG_EXECUTABLE}")
        message(FATAL_ERROR
            "No se encontró llvm-config en '${LLVM_STATIC_ROOT}/bin/'. "
            "Verificá que LLVM_STATIC_ROOT apunte al directorio de instalación "
            "(el que se pasó como CMAKE_INSTALL_PREFIX al compilar LLVM).")
    endif()

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)

    message(STATUS "LLVM (ESTÁTICO) version: ${LLVM_VERSION}")
    message(STATUS "LLVM (ESTÁTICO) root:    ${LLVM_STATIC_ROOT}")

    include_directories(${LLVM_INCLUDE_DIRS})

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

else()
    # ── Modo dinámico (default — desarrollo local) ──────────────────────────
    # En Arch Linux, LLVM se distribuye como librería unificada (libLLVM.so).
    # llvm_map_components_to_libnames() produce -lLLVMCore etc. que NO existen
    # como .so individuales — por eso usamos llvm-config directamente.
    find_package(LLVM REQUIRED CONFIG)

    message(STATUS "LLVM (dinámico) version: ${LLVM_VERSION}")
    message(STATUS "LLVM (dinámico) include dirs: ${LLVM_INCLUDE_DIRS}")

    include_directories(${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    add_definitions(${LLVM_DEFINITIONS_LIST})

    find_program(LLVM_CONFIG_EXECUTABLE
        NAMES llvm-config llvm-config-22 llvm-config-18 llvm-config-17
        REQUIRED
    )
endif()

# ── Flags de linking (comunes a ambos modos vía llvm-config) ──────────────────
# --libs: lista de -lLLVMXxx (estático) o -lLLVM-22 (dinámico)
execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core orcjit native analysis passes irreader
    OUTPUT_VARIABLE LLVM_LINK_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# --system-libs: dependencias del sistema que LLVM necesita (zlib, pthread, etc.)
execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LD_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(LLVM_LINK_FLAGS_LIST   NATIVE_COMMAND "${LLVM_LINK_FLAGS}")
separate_arguments(LLVM_SYSTEM_LIBS_LIST  NATIVE_COMMAND "${LLVM_SYSTEM_LIBS}")
separate_arguments(LLVM_LD_FLAGS_LIST     NATIVE_COMMAND "${LLVM_LD_FLAGS}")

message(STATUS "LLVM link flags: ${LLVM_LINK_FLAGS}")

# ── Tests globales ────────────────────────────────────────────────────────────
enable_testing()
include(CTest)

# Crear directorios de trabajo para los tests
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lexer)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/parser)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/semantic)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/codegen)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/integration)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/errors)

# ── CLI ───────────────────────────────────────────────────────────────────────
add_subdirectory(cli)

# ── libkem ────────────────────────────────────────────────────────────────────
add_library(libkem STATIC
    src/Token.cpp
    src/ErrorHandler.cpp
    src/ErrorMessages.cpp
    src/LangConfig.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/SemanticAnalyzer.cpp
    src/IRGenerator.cpp
    src/JITEngine.cpp
)

set_target_properties(libkem PROPERTIES OUTPUT_NAME "kem")

target_include_directories(libkem PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Linkear con los flags reales de llvm-config
target_link_libraries(libkem PUBLIC
    ${LLVM_LD_FLAGS_LIST}
    ${LLVM_LINK_FLAGS_LIST}
    ${LLVM_SYSTEM_LIBS_LIST}
)

# ── Binario completamente estático (solo en modo distribución) ───────────────
# Sin esto, aunque LLVM esté linkeado estáticamente, el ejecutable `kem`
# seguiría dependiendo de libstdc++.so y libgcc_s.so del sistema — lo cual
# rompe el objetivo de "el estudiante descarga y ejecuta sin instalar nada"
# si su sistema tiene una versión distinta de esas libs.
#
# -static-libgcc / -static-libstdc++: empaquetan el runtime de C++ adentro
# del binario. No se usa -static completo (libc estática) porque eso
# rompe getaddrinfo/dlopen en Linux con glibc — el runtime del JIT de LLVM
# usa dlopen internamente para resolver símbolos, así que glibc se mantiene
# dinámica (siempre presente en cualquier Linux) y solo el C++ runtime
# se empaqueta estático.
if(KEM_STATIC_LLVM AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(libkem PUBLIC -static-libgcc -static-libstdc++)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(KEM_BUILD_TESTS "Compilar los tests" ON)
if(KEM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ── Copiar langs/ al build ────────────────────────────────────────────────────
add_custom_target(copy_langs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/cli/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/lexer/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/parser/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/semantic/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/codegen/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/integration/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs ${CMAKE_BINARY_DIR}/tests/errors/langs
    COMMENT "Copiando archivos de idioma al directorio de build"
)

message(STATUS "")
message(STATUS "── KEM build configuration ─────────────────")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "  LLVM:       ${LLVM_VERSION}")
if(KEM_STATIC_LLVM)
    message(STATUS "  Modo LLVM:  ESTÁTICO (binario distribuible)")
    message(STATUS "  LLVM root:  ${LLVM_STATIC_ROOT}")
else()
    message(STATUS "  Modo LLVM:  dinámico (desarrollo local)")
endif()
message(STATUS "  llvm-config: ${LLVM_CONFIG_EXECUTABLE}")
message(STATUS "  Tests:      ${KEM_BUILD_TESTS}")
message(STATUS "────────────────────────────────────────────")
message(STATUS "")
