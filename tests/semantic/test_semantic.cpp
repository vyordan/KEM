#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include "kem/Token.hpp"
#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/SemanticAnalyzer.hpp"
#include "kem/ErrorHandler.hpp"

// ─────────────────────────────────────────────
//  Mini framework
// ─────────────────────────────────────────────
static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) \
    void name(); \
    struct _reg_##name { _reg_##name() { \
        ++tests_run; \
        try { name(); ++tests_passed; std::cout << "  \xE2\x9C\x93 " #name "\n"; } \
        catch (const std::exception& e) { \
            ++tests_failed; std::cout << "  \xE2\x9C\x97 " #name ": " << e.what() << "\n"; } \
    }} _inst_##name; \
    void name()

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE falló: " #expr)

#define ASSERT_OK(src) \
    do { \
        try { analyze(src); } \
        catch (const kem::KemError& e) { \
            throw std::runtime_error(std::string("Se esperaba OK pero lanzó: ") + e.what()); \
        } \
    } while(0)

#define ASSERT_ERROR(src) \
    do { \
        bool _threw = false; \
        try { analyze(src); } \
        catch (const kem::KemError&) { _threw = true; } \
        if (!_threw) throw std::runtime_error("Se esperaba error semántico pero no lanzó"); \
    } while(0)

// ─────────────────────────────────────────────
//  Helper — lexea, parsea y analiza
// ─────────────────────────────────────────────
static kem::LangConfig& cfg() {
    static kem::LangConfig c("langs/espanol.json");
    return c;
}

static void analyze(const std::string& src) {
    kem::Lexer lexer(src, cfg());
    auto tokens = lexer.tokenize();
    kem::Parser parser(std::move(tokens));
    auto prog = parser.parse();
    kem::SemanticAnalyzer sem;
    sem.analyze(*prog);
}

// ─────────────────────────────────────────────
//  Tests — programas válidos
// ─────────────────────────────────────────────

TEST(test_programa_minimo) {
    ASSERT_OK("inicio {}");
}

TEST(test_declaracion_con_init) {
    ASSERT_OK("inicio { entero x = 5 }");
}

TEST(test_declaracion_decimal) {
    ASSERT_OK("inicio { decimal x = 3.14 }");
}

TEST(test_declaracion_booleano) {
    ASSERT_OK("inicio { booleano b = verdadero }");
}

TEST(test_declaracion_sin_init) {
    ASSERT_OK("inicio { entero x }");
}

TEST(test_arreglo_valido) {
    ASSERT_OK("inicio { entero nums[3] = [1, 2, 3] }");
}

TEST(test_asignacion_valida) {
    ASSERT_OK("inicio { entero x\n x = 10 }");
}

TEST(test_expr_aritmetica) {
    ASSERT_OK("inicio { entero r = 2 + 3 * 4 }");
}

TEST(test_expr_mixta_entero_decimal) {
    ASSERT_OK("inicio { decimal r = 2 + 3.14 }");
}

TEST(test_expr_booleana) {
    ASSERT_OK("inicio { booleano b = verdadero y falso }");
}

TEST(test_expr_relacional) {
    ASSERT_OK("inicio { booleano b = 3 >= 2 }");
}

TEST(test_negacion_unaria) {
    ASSERT_OK("inicio { entero x = -5 }");
}

TEST(test_no_unario) {
    ASSERT_OK("inicio { booleano b = no verdadero }");
}

TEST(test_si_valido) {
    ASSERT_OK("inicio {\n entero x = 5\n si x > 0 {\n entero y = 1\n }\n}");
}

TEST(test_mientras_valido) {
    ASSERT_OK("inicio { booleano activo = verdadero\n mientras activo { activo = falso } }");
}

TEST(test_hasta_valido) {
    ASSERT_OK("inicio { entero i\n i = 0 hasta 10 { entero x = i } }");
}

TEST(test_hasta_con_paso) {
    ASSERT_OK("inicio { entero i\n i = 0 hasta 100 paso 5 { } }");
}

TEST(test_funcion_simple) {
    ASSERT_OK(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(1, 2) }");
}

TEST(test_funcion_recursiva) {
    ASSERT_OK(
        "funcion fib(entero n) entero {\n"
        "  si n < 2 { devolver n }\n"
        "  devolver fib(n - 1) + fib(n - 2)\n"
        "}\n"
        "inicio { entero r = fib(10) }");
}

TEST(test_procedimiento_valido) {
    ASSERT_OK(
        "procedimiento saludar(entero x) {\n entero y = x + 1\n}\ninicio { saludar(5) }");
}

TEST(test_enlazar_valido) {
    ASSERT_OK("enlazar vacio imprimir(entero)\ninicio { imprimir(42) }");
}

TEST(test_arreglo_acceso_valido) {
    ASSERT_OK("inicio { entero nums[3] = [1, 2, 3]\n entero x = nums[0] }");
}

TEST(test_forward_declaration) {
    // f2 llama a f1 que está definida después
    ASSERT_OK(
        "funcion f1(entero x) entero { devolver x }\n"
        "funcion f2(entero x) entero { devolver f1(x) }\n"
        "inicio { entero r = f2(5) }");
}

// ─────────────────────────────────────────────
//  Tests — errores semánticos
// ─────────────────────────────────────────────

TEST(test_error_variable_no_declarada) {
    ASSERT_ERROR("inicio { entero x = y }");
}

TEST(test_error_variable_duplicada) {
    ASSERT_ERROR("inicio { entero x = 1\n entero x = 2 }");
}

TEST(test_error_tipo_incompatible_init) {
    ASSERT_ERROR("inicio { entero x = verdadero }");
}

TEST(test_error_tipo_incompatible_asign) {
    ASSERT_ERROR("inicio { entero x = 5\n x = verdadero }");
}

TEST(test_error_suma_con_booleano) {
    ASSERT_ERROR("inicio { entero x = 5 + verdadero }");
}

TEST(test_error_y_con_entero) {
    ASSERT_ERROR("inicio { booleano b = 5 y verdadero }");
}

TEST(test_error_no_sobre_entero) {
    ASSERT_ERROR("inicio { booleano b = no 5 }");
}

TEST(test_error_si_condicion_no_booleana) {
    ASSERT_ERROR("inicio { entero x = 5\n si x { } }");
}

TEST(test_error_mientras_condicion_no_booleana) {
    ASSERT_ERROR("inicio { entero x = 5\n mientras x { } }");
}

TEST(test_error_funcion_no_retorna) {
    ASSERT_ERROR("funcion f(entero x) entero { entero y = x }\ninicio {}");
}

TEST(test_error_procedimiento_retorna_valor) {
    ASSERT_ERROR(
        "procedimiento f(entero x) { devolver x }\n"
        "inicio {}");
}

TEST(test_error_funcion_no_declarada) {
    ASSERT_ERROR("inicio { entero r = noExiste(5) }");
}

TEST(test_error_argumentos_insuficientes) {
    ASSERT_ERROR(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(1) }");
}

TEST(test_error_argumentos_de_mas) {
    ASSERT_ERROR(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(1, 2, 3) }");
}

TEST(test_error_tipo_argumento_incorrecto) {
    ASSERT_ERROR(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(1, verdadero) }");
}

TEST(test_error_indice_no_entero) {
    ASSERT_ERROR("inicio { entero nums[3]\n entero x = nums[verdadero] }");
}

TEST(test_error_indexar_escalar) {
    ASSERT_ERROR("inicio { entero x = 5\n entero y = x[0] }");
}

TEST(test_error_itervar_no_declarada) {
    ASSERT_ERROR("inicio { i = 0 hasta 10 { } }");
}

TEST(test_error_itervar_no_entero) {
    ASSERT_ERROR("inicio { decimal i\n i = 0.0 hasta 10.0 { } }");
}

TEST(test_error_arreglo_init_demasiado_grande) {
    ASSERT_ERROR("inicio { entero nums[2] = [1, 2, 3] }");
}

TEST(test_error_struct_campo_duplicado) {
    ASSERT_ERROR(
        "estructura Punto { entero x\n entero x }\n"
        "inicio {}");
}

// ─────────────────────────────────────────────
//  Tests — scopes
// ─────────────────────────────────────────────

TEST(test_scope_variable_local) {
    // variable declarada en un bloque no se ve afuera — pero el semántico
    // no lanza aquí porque no hay acceso fuera del bloque
    ASSERT_OK("inicio { si verdadero { entero x = 5 } }");
}

TEST(test_scope_mismo_nombre_distinto_bloque) {
    // x en dos bloques distintos es válido
    ASSERT_OK(
        "inicio {\n"
        "  si verdadero { entero x = 1 }\n"
        "  si verdadero { entero x = 2 }\n"
        "}");
}

TEST(test_scope_funcion_vs_global) {
    // misma variable en función y en inicio — distintos scopes, OK
    ASSERT_OK(
        "funcion f(entero x) entero { entero resultado = x\n devolver resultado }\n"
        "inicio { entero resultado = f(5) }");
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n\xE2\x94\x80\xE2\x94\x80 Tests del Analizador Sem\xC3\xA1ntico de KEM "
                 "\xE2\x94\x80\xE2\x94\x80\n\n";
    std::cout << "\n\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
