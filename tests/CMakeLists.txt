enable_testing()

# ── Lexer ─────────────────────────────────────
add_executable(kem_test_lexer lexer/test_tokens.cpp)
target_link_libraries(kem_test_lexer PRIVATE libkem)
add_dependencies(kem_test_lexer copy_langs)
add_test(NAME lexer_tokens COMMAND kem_test_lexer
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lexer)

# ── Parser ────────────────────────────────────
add_executable(kem_test_parser parser/test_parser.cpp)
target_link_libraries(kem_test_parser PRIVATE libkem)
add_dependencies(kem_test_parser copy_langs)
add_test(NAME parser_ast COMMAND kem_test_parser
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/parser)

# ── Semántico ─────────────────────────────────
add_executable(kem_test_semantic semantic/test_semantic.cpp)
target_link_libraries(kem_test_semantic PRIVATE libkem)
add_dependencies(kem_test_semantic copy_langs)
add_test(NAME semantic COMMAND kem_test_semantic
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/semantic)

# ── Codegen + JIT ─────────────────────────────
add_executable(kem_test_codegen codegen/test_ir.cpp)
target_link_libraries(kem_test_codegen PRIVATE libkem)
add_dependencies(kem_test_codegen copy_langs)
add_test(NAME codegen_ir COMMAND kem_test_codegen
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/codegen)

# ── Integración end-to-end ────────────────────
add_executable(kem_test_integration integration/test_ejemplos.cpp)
target_link_libraries(kem_test_integration PRIVATE libkem)
add_dependencies(kem_test_integration copy_langs)
add_test(NAME integration COMMAND kem_test_integration
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/integration)
