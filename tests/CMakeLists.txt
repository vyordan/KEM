# ─────────────────────────────────────────────────────────────────────────────
#  Tests de KEM
#
#  Los tests de Lexer, Parser y Semántico NO necesitan LLVM —
#  se linkean contra una versión sin LLVM de libkem para compilar más rápido
#  y no requerir LLVM en el entorno de CI si no está disponible.
#
#  Los tests de Codegen e Integración SÍ necesitan LLVM (usan IRGenerator/JIT).
# ─────────────────────────────────────────────────────────────────────────────

# ── libkem_base: módulos sin LLVM (Lexer, Parser, Semántico) ─────────────────
# Evita linkear LLVM en los tests que no lo necesitan
add_library(libkem_base STATIC
    ${CMAKE_SOURCE_DIR}/src/Token.cpp
    ${CMAKE_SOURCE_DIR}/src/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/LangConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/Lexer.cpp
    ${CMAKE_SOURCE_DIR}/src/AST.cpp
    ${CMAKE_SOURCE_DIR}/src/Parser.cpp
    ${CMAKE_SOURCE_DIR}/src/SemanticAnalyzer.cpp
)
target_include_directories(libkem_base PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ── Lexer ─────────────────────────────────────────────────────────────────────
add_executable(kem_test_lexer lexer/test_tokens.cpp)
target_link_libraries(kem_test_lexer PRIVATE libkem_base)
add_dependencies(kem_test_lexer copy_langs)
add_test(
    NAME    lexer_tokens
    COMMAND $<TARGET_FILE:kem_test_lexer>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lexer
)

# ── Parser ────────────────────────────────────────────────────────────────────
add_executable(kem_test_parser parser/test_parser.cpp)
target_link_libraries(kem_test_parser PRIVATE libkem_base)
add_dependencies(kem_test_parser copy_langs)
add_test(
    NAME    parser_ast
    COMMAND $<TARGET_FILE:kem_test_parser>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/parser
)

# ── Semántico ─────────────────────────────────────────────────────────────────
add_executable(kem_test_semantic semantic/test_semantic.cpp)
target_link_libraries(kem_test_semantic PRIVATE libkem_base)
add_dependencies(kem_test_semantic copy_langs)
add_test(
    NAME    semantic
    COMMAND $<TARGET_FILE:kem_test_semantic>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/semantic
)

# ── Codegen + JIT (requiere LLVM) ─────────────────────────────────────────────
add_executable(kem_test_codegen codegen/test_ir.cpp)
target_link_libraries(kem_test_codegen PRIVATE libkem)
add_dependencies(kem_test_codegen copy_langs)
add_test(
    NAME    codegen_ir
    COMMAND $<TARGET_FILE:kem_test_codegen>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/codegen
)

# ── Integración end-to-end (requiere LLVM) ────────────────────────────────────
add_executable(kem_test_integration integration/test_ejemplos.cpp)
target_link_libraries(kem_test_integration PRIVATE libkem)
add_dependencies(kem_test_integration copy_langs)
add_test(
    NAME    integration
    COMMAND $<TARGET_FILE:kem_test_integration>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/integration
)
