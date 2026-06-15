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
# En Arch Linux, LLVM se distribuye como una librería unificada (libLLVM.so).
# llvm_map_components_to_libnames() produce -lLLVMCore etc. que NO existen.
# Usamos llvm-config directamente para obtener los flags correctos.
find_package(LLVM REQUIRED CONFIG)

message(STATUS "LLVM version: ${LLVM_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Obtener flags de linking via llvm-config (compatible con Arch y otras distros)
find_program(LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config llvm-config-22 llvm-config-18 llvm-config-17
    REQUIRED
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core orcjit native analysis passes irreader
    OUTPUT_VARIABLE LLVM_LINK_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

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

# Separar los flags en listas para CMake
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

# ── CLI ───────────────────────────────────────────────────────────────────────
add_subdirectory(cli)

# ── libkem ────────────────────────────────────────────────────────────────────
add_library(libkem STATIC
    src/Token.cpp
    src/ErrorHandler.cpp
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
    COMMENT "Copiando archivos de idioma al directorio de build"
)

message(STATUS "")
message(STATUS "── KEM build configuration ─────────────────")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "  LLVM:       ${LLVM_VERSION}")
message(STATUS "  llvm-config: ${LLVM_CONFIG_EXECUTABLE}")
message(STATUS "  Tests:      ${KEM_BUILD_TESTS}")
message(STATUS "────────────────────────────────────────────")
message(STATUS "")
