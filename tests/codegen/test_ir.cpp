#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "kem/Token.hpp"
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

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE fallo: " #expr)

static kem::LangConfig& cfg() {
    static kem::LangConfig c("langs/espanol.json");
    return c;
}

static void compile(const std::string& src) {
    kem::Lexer lexer(src, cfg());
    auto tokens = lexer.tokenize();
    kem::Parser parser(std::move(tokens));
    auto prog = parser.parse();
    kem::SemanticAnalyzer sem;
    sem.analyze(*prog);
    kem::IRGenerator gen;
    gen.generate(*prog);
}

static int64_t run(const std::string& src) {
    kem::Lexer lexer(src, cfg());
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

// ── Tests generales de IR ────────────────────────────────────────────────────

TEST(test_inicio_vacio) {
    ASSERT_TRUE(run("inicio {}") == 0);
}

TEST(test_funcion_simple) {
    compile(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio {}");
}

TEST(test_factorial) {
    compile(
        "funcion factorial(entero n) entero {\n"
        "    si n == 0 { devolver 1 }\n"
        "    devolver n * factorial(n - 1)\n"
        "}\n"
        "inicio {}");
}

TEST(test_mientras) {
    compile(
        "inicio {\n"
        "    entero i\n"
        "    entero suma = 0\n"
        "    mientras i < 10 {\n"
        "        suma = suma + i\n"
        "        i = i + 1\n"
        "    }\n"
        "}");
}

TEST(test_hasta) {
    compile(
        "inicio {\n"
        "    entero i\n"
        "    entero suma = 0\n"
        "    i = 0 hasta 10 {\n"
        "        suma = suma + i\n"
        "    }\n"
        "}");
}

TEST(test_arreglo) {
    compile(
        "inicio {\n"
        "    entero nums[5] = [10, 20, 30, 40, 50]\n"
        "    entero x = nums[2]\n"
        "}");
}

TEST(test_si_sino) {
    compile(
        "funcion max(entero a, entero b) entero {\n"
        "    si a > b { devolver a }\n"
        "    sino { devolver b }\n"
        "}\n"
        "inicio {}");
}

TEST(test_procedimiento) {
    compile(
        "procedimiento noop(entero x) { entero y = x + 1 }\n"
        "inicio { noop(42) }");
}

TEST(test_booleanos) {
    compile(
        "inicio {\n"
        "    booleano a = verdadero\n"
        "    booleano b = falso\n"
        "    booleano c = a y b\n"
        "    booleano d = no a\n"
        "    booleano e = 5 > 3\n"
        "}");
}

TEST(test_decimal) {
    compile(
        "funcion area(decimal base, decimal altura) decimal {\n"
        "    devolver base * altura / 2.0\n"
        "}\n"
        "inicio { decimal r = area(5.0, 3.0) }");
}

TEST(test_fibonacci) {
    compile(
        "funcion fib(entero n) entero {\n"
        "    si n < 2 { devolver n }\n"
        "    devolver fib(n - 1) + fib(n - 2)\n"
        "}\n"
        "inicio { entero r = fib(10) }");
}

// ── Tests de builtins de consola ─────────────────────────────────────────────

TEST(test_builtin_imprimirEntero_compila) {
    compile(
        "inicio {\n"
        "    imprimirEntero(42)\n"
        "}");
}

TEST(test_builtin_imprimirDecimal_compila) {
    compile(
        "inicio {\n"
        "    imprimirDecimal(3.14)\n"
        "}");
}

TEST(test_builtin_imprimir_compila) {
    compile(
        "inicio {\n"
        "    imprimir(\"hola\")\n"
        "}");
}

TEST(test_builtin_imprimirLinea_compila) {
    compile(
        "inicio {\n"
        "    imprimirLinea(\"hola mundo\")\n"
        "}");
}

TEST(test_builtin_leerEntero_compila) {
    compile(
        "inicio {\n"
        "    entero x = leerEntero()\n"
        "    imprimirEntero(x)\n"
        "}");
}

TEST(test_builtin_leerDecimal_compila) {
    compile(
        "inicio {\n"
        "    decimal d = leerDecimal()\n"
        "    imprimirDecimal(d)\n"
        "}");
}

TEST(test_builtin_leerLinea_compila) {
    compile(
        "inicio {\n"
        "    texto t = leerLinea()\n"
        "    imprimirLinea(t)\n"
        "}");
}

TEST(test_builtin_imprimirEntero_con_calculo) {
    compile(
        "funcion factorial(entero n) entero {\n"
        "    si n == 0 { devolver 1 }\n"
        "    devolver n * factorial(n - 1)\n"
        "}\n"
        "inicio {\n"
        "    imprimirEntero(factorial(5))\n"
        "}");
}

TEST(test_builtin_multiple_salida) {
    compile(
        "inicio {\n"
        "    imprimir(\"a = \")\n"
        "    imprimirEntero(42)\n"
        "    imprimir(\"pi = \")\n"
        "    imprimirDecimal(3.14)\n"
        "    imprimirLinea(\"fin\")\n"
        "}");
}

TEST(test_builtin_en_funcion) {
    compile(
        "procedimiento mostrar(entero x) {\n"
        "    imprimir(\"valor: \")\n"
        "    imprimirEntero(x)\n"
        "}\n"
        "inicio {\n"
        "    mostrar(100)\n"
        "    mostrar(200)\n"
        "}");
}

TEST(test_builtin_en_loop) {
    compile(
        "inicio {\n"
        "    entero i\n"
        "    i = 1 hasta 6 {\n"
        "        imprimirEntero(i)\n"
        "    }\n"
        "}");
}

TEST(test_builtin_con_si) {
    compile(
        "inicio {\n"
        "    entero x = 10\n"
        "    si x > 5 {\n"
        "        imprimirLinea(\"mayor que 5\")\n"
        "    } sino {\n"
        "        imprimirLinea(\"menor o igual a 5\")\n"
        "    }\n"
        "}");
}

TEST(test_semantico_rechaza_leerEntero_con_args) {
    bool lanzo = false;
    try {
        compile("inicio { entero x = leerEntero(42) }");
    } catch (const kem::KemError&) { lanzo = true; }
    ASSERT_TRUE(lanzo);
}

TEST(test_semantico_rechaza_imprimirEntero_sin_args) {
    bool lanzo = false;
    try {
        compile("inicio { imprimirEntero() }");
    } catch (const kem::KemError&) { lanzo = true; }
    ASSERT_TRUE(lanzo);
}

// ── Tests de ejecucion con builtins ─────────────────────────────────────────
// Estos tests ejecutan el JIT — la salida va a stdout del test runner

TEST(test_ejecucion_imprimirEntero) {
    // Ejecuta sin lanzar y produce output
    int64_t r = run(
        "inicio {\n"
        "    imprimirEntero(42)\n"
        "}");
    ASSERT_TRUE(r == 0);
}

TEST(test_ejecucion_imprimirDecimal) {
    int64_t r = run(
        "inicio {\n"
        "    imprimirDecimal(3.14)\n"
        "}");
    ASSERT_TRUE(r == 0);
}

TEST(test_ejecucion_imprimirLinea) {
    int64_t r = run(
        "inicio {\n"
        "    imprimirLinea(\"KEM funciona!\")\n"
        "}");
    ASSERT_TRUE(r == 0);
}

TEST(test_ejecucion_factorial_con_salida) {
    int64_t r = run(
        "funcion factorial(entero n) entero {\n"
        "    si n == 0 { devolver 1 }\n"
        "    devolver n * factorial(n - 1)\n"
        "}\n"
        "inicio {\n"
        "    imprimirEntero(factorial(10))\n"
        "}");
    ASSERT_TRUE(r == 0);
}

TEST(test_ejecucion_loop_con_salida) {
    int64_t r = run(
        "inicio {\n"
        "    entero i\n"
        "    entero suma = 0\n"
        "    i = 1 hasta 6 {\n"
        "        suma = suma + i\n"
        "    }\n"
        "    imprimirEntero(suma)\n"
        "}");
    ASSERT_TRUE(r == 0);
}

int main() {
    std::cout << "\n-- Tests IRGenerator + Builtins KEM --\n\n";
    std::cout << "\n-----\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
