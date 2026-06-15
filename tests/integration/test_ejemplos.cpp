#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/SemanticAnalyzer.hpp"
#include "kem/IRGenerator.hpp"
#include "kem/JITEngine.hpp"
#include "kem/ErrorHandler.hpp"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) \
    void name(); \
    struct _reg_##name { _reg_##name() { \
        ++tests_run; \
        try { name(); ++tests_passed; std::cout << "  OK " #name "\n"; } \
        catch (const std::exception& e) { \
            ++tests_failed; \
            std::cout << "  FAIL " #name ": " << e.what() << "\n"; } \
    }} _inst_##name; \
    void name()

#define ASSERT_EQ(a, b) \
    do { auto _a=(a); auto _b=(b); \
         if (_a!=_b) { std::ostringstream _s; \
                       _s << "ASSERT_EQ: " << _a << " != " << _b; \
                       throw std::runtime_error(_s.str()); } } while(0)

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE fallo: " #expr)

static kem::LangConfig& cfg_es() {
    static kem::LangConfig c("langs/espanol.json");
    return c;
}

static kem::LangConfig& cfg_en() {
    static kem::LangConfig c("langs/english.json");
    return c;
}

static int64_t execute(const std::string& src, kem::LangConfig& cfg) {
    kem::Lexer lexer(src, cfg);
    auto tokens = lexer.tokenize();
    kem::Parser parser(std::move(tokens));
    auto prog = parser.parse();
    kem::SemanticAnalyzer sem;
    sem.analyze(*prog);
    kem::IRGenerator gen;
    gen.generate(*prog);
    kem::JITEngine jit;
    jit.addModule(gen.takeModule(), gen.context());
    return jit.run();
}

static int64_t run(const std::string& src) { return execute(src, cfg_es()); }
static int64_t run_en(const std::string& src) { return execute(src, cfg_en()); }

// ── Programa minimo ──────────────────────────────────────────────────────────

TEST(test_inicio_vacio) {
    ASSERT_EQ(run("inicio {}"), 0LL);
}

// ── Aritmetica ───────────────────────────────────────────────────────────────

TEST(test_suma_compila) {
    int64_t r = run(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(3, 4) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_fibonacci_compila) {
    int64_t r = run(
        "funcion fib(entero n) entero {\n"
        "    si n < 2 { devolver n }\n"
        "    devolver fib(n - 1) + fib(n - 2)\n"
        "}\n"
        "inicio { entero r = fib(10) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_factorial_compila) {
    int64_t r = run(
        "funcion factorial(entero n) entero {\n"
        "    si n == 0 { devolver 1 }\n"
        "    devolver n * factorial(n - 1)\n"
        "}\n"
        "inicio { entero r = factorial(5) }");
    ASSERT_TRUE(r >= 0);
}

// ── Flujo de control ─────────────────────────────────────────────────────────

TEST(test_si_sino) {
    int64_t r = run(
        "funcion signo(entero x) entero {\n"
        "    si x > 0 { devolver 1 }\n"
        "    sino si x < 0 { devolver -1 }\n"
        "    devolver 0\n"
        "}\n"
        "inicio { entero r = signo(5) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_mientras) {
    int64_t r = run(
        "funcion suma_hasta(entero n) entero {\n"
        "    entero suma = 0\n"
        "    entero i = 1\n"
        "    mientras i <= n {\n"
        "        suma = suma + i\n"
        "        i = i + 1\n"
        "    }\n"
        "    devolver suma\n"
        "}\n"
        "inicio { entero r = suma_hasta(10) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_hasta) {
    int64_t r = run(
        "funcion suma_arr() entero {\n"
        "    entero nums[5] = [10, 20, 30, 40, 50]\n"
        "    entero suma = 0\n"
        "    entero i\n"
        "    i = 0 hasta 5 {\n"
        "        suma = suma + nums[i]\n"
        "    }\n"
        "    devolver suma\n"
        "}\n"
        "inicio { entero r = suma_arr() }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_hasta_con_paso) {
    int64_t r = run(
        "funcion suma_pares() entero {\n"
        "    entero suma = 0\n"
        "    entero i\n"
        "    i = 0 hasta 10 paso 2 {\n"
        "        suma = suma + i\n"
        "    }\n"
        "    devolver suma\n"
        "}\n"
        "inicio { entero r = suma_pares() }");
    ASSERT_TRUE(r >= 0);
}

// ── Arreglos ─────────────────────────────────────────────────────────────────

TEST(test_arreglo_acceso) {
    int64_t r = run(
        "funcion get_elem() entero {\n"
        "    entero nums[3] = [10, 20, 30]\n"
        "    devolver nums[1]\n"
        "}\n"
        "inicio { entero r = get_elem() }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_arreglo_modificacion) {
    int64_t r = run(
        "funcion test() entero {\n"
        "    entero nums[3] = [1, 2, 3]\n"
        "    nums[0] = 99\n"
        "    devolver nums[0]\n"
        "}\n"
        "inicio { entero r = test() }");
    ASSERT_TRUE(r >= 0);
}

// ── Procedimientos ───────────────────────────────────────────────────────────

TEST(test_procedimiento) {
    int64_t r = run(
        "procedimiento noop(entero x) { entero y = x + 1 }\n"
        "inicio { noop(42) }");
    ASSERT_EQ(r, 0LL);
}

// ── Tipos ────────────────────────────────────────────────────────────────────

TEST(test_decimal) {
    int64_t r = run(
        "funcion area(decimal base, decimal altura) decimal {\n"
        "    devolver base * altura / 2.0\n"
        "}\n"
        "inicio { decimal r = area(5.0, 3.0) }");
    ASSERT_EQ(r, 0LL);
}

TEST(test_booleano) {
    int64_t r = run(
        "funcion es_mayor(entero a, entero b) booleano {\n"
        "    devolver a > b\n"
        "}\n"
        "inicio { booleano b = es_mayor(5, 3) }");
    ASSERT_EQ(r, 0LL);
}

// ── Multi-idioma (english.json) ───────────────────────────────────────────────

TEST(test_english_suma) {
    int64_t r = run_en(
        "function suma(int a, int b) int {\n"
        "    return a + b\n"
        "}\n"
        "main { int r = suma(3, 4) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_english_factorial) {
    int64_t r = run_en(
        "function factorial(int n) int {\n"
        "    if n == 0 { return 1 }\n"
        "    return n * factorial(n - 1)\n"
        "}\n"
        "main { int r = factorial(5) }");
    ASSERT_TRUE(r >= 0);
}

TEST(test_english_while) {
    int64_t r = run_en(
        "function f() int {\n"
        "    int suma = 0\n"
        "    int i = 0\n"
        "    while i < 5 {\n"
        "        suma = suma + i\n"
        "        i = i + 1\n"
        "    }\n"
        "    return suma\n"
        "}\n"
        "main { int r = f() }");
    ASSERT_TRUE(r >= 0);
}

// ── Errores en compilacion ────────────────────────────────────────────────────

TEST(test_error_variable_no_declarada) {
    bool lanzo = false;
    try { run("inicio { entero x = z }"); }
    catch (const kem::KemError&) { lanzo = true; }
    ASSERT_TRUE(lanzo);
}

TEST(test_error_tipo_incompatible) {
    bool lanzo = false;
    try { run("inicio { entero x = verdadero }"); }
    catch (const kem::KemError&) { lanzo = true; }
    ASSERT_TRUE(lanzo);
}

TEST(test_error_funcion_sin_return) {
    bool lanzo = false;
    try { run("funcion f(entero x) entero { entero y = x }\ninicio {}"); }
    catch (const kem::KemError&) { lanzo = true; }
    ASSERT_TRUE(lanzo);
}

int main() {
    std::cout << "\n-- Tests de Integracion KEM --\n\n";
    std::cout << "\n-----\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
