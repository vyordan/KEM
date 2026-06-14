cmake_minimum_required(VERSION 3.20)
project(kem VERSION 0.1.0 LANGUAGES C CXX)   # Añadido C para que LLVM pueda comprobar histedit.h

# ── Estándar C++ ───────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Flags de compilación ───────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter   # silencia params sin usar durante el desarrollo
    )
endif()

# ── Tipo de build por defecto ──────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ── LLVM ───────────────────────────────────────────────────────────────────────
# Requiere LLVM instalado: sudo pacman -S llvm
# Verificar versión instalada: llvm-config --version
find_package(LLVM REQUIRED CONFIG)

message(STATUS "LLVM version: ${LLVM_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Obtener las bibliotecas de LLVM usando llvm-config (funciona con versionado)
execute_process(
    COMMAND llvm-config --libs core
    OUTPUT_VARIABLE LLVM_LIBS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_LIBS UNIX_COMMAND ${LLVM_LIBS_RAW})

# ── libkem (librería estática) ─────────────────────────────────────────────────
# Todo el compilador vive aquí. El CLI, los tests, y en el futuro
# grpc/ y web/ linkean contra libkem — sin duplicar código.
add_library(libkem STATIC
    src/Token.cpp
    src/ErrorHandler.cpp
    src/LangConfig.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/SemanticAnalyzer.cpp
    # src/IRGenerator.cpp
    # src/JITEngine.cpp
)

# Nombre sin el prefijo 'lib' para claridad
set_target_properties(libkem PROPERTIES OUTPUT_NAME "kem")

target_include_directories(libkem PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(libkem PUBLIC
    ${LLVM_LIBS}
)

# ── CLI ────────────────────────────────────────────────────────────────────────
add_subdirectory(cli)

# ── Tests ──────────────────────────────────────────────────────────────────────
option(KEM_BUILD_TESTS "Compilar los tests" ON)
if(KEM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ── Copiar langs/ al directorio de build ──────────────────────────────────────
# Para que ./kem pueda encontrar langs/espanol.json sin rutas absolutas
add_custom_target(copy_langs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs
        ${CMAKE_BINARY_DIR}/cli/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs
        ${CMAKE_BINARY_DIR}/tests/lexer/langs
    COMMENT "Copiando archivos de idioma al directorio de build"
)

# ── Información del build ──────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "── KEM build configuration ─────────────────")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std:     C++${CMAKE_CXX_STANDARD}")
message(STATUS "  LLVM:        ${LLVM_VERSION}")
message(STATUS "  Tests:       ${KEM_BUILD_TESTS}")
message(STATUS "────────────────────────────────────────────")
message(STATUS "")