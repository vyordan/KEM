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

// Compila un programa KEM y verifica que no lanza
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

// Compila y ejecuta, retorna resultado de inicio{}
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

// ── Tests de ejecucion ───────────────────────────────────────────────────────

TEST(test_retorna_cero_por_defecto) {
    int64_t r = run("inicio {}");
    ASSERT_TRUE(r == 0);
}

TEST(test_funcion_suma_compila) {
    compile(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio {}");
}

TEST(test_inicio_llama_funcion) {
    int64_t r = run(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(3, 4) }");
    ASSERT_TRUE(r == 0);
}

// ── Tests de compilacion a IR ─────────────────────────────────────────────────

TEST(test_ir_funcion_simple) {
    compile(
        "funcion suma(entero a, entero b) entero {\n"
        "    devolver a + b\n"
        "}\n"
        "inicio {}");
}

TEST(test_ir_factorial) {
    compile(
        "funcion factorial(entero n) entero {\n"
        "    si n == 0 {\n"
        "        devolver 1\n"
        "    }\n"
        "    devolver n * factorial(n - 1)\n"
        "}\n"
        "inicio {}");
}

TEST(test_ir_mientras) {
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

TEST(test_ir_hasta) {
    compile(
        "inicio {\n"
        "    entero i\n"
        "    entero suma = 0\n"
        "    i = 0 hasta 10 {\n"
        "        suma = suma + i\n"
        "    }\n"
        "}");
}

TEST(test_ir_hasta_con_paso) {
    compile(
        "inicio {\n"
        "    entero i\n"
        "    entero suma = 0\n"
        "    i = 0 hasta 100 paso 5 {\n"
        "        suma = suma + i\n"
        "    }\n"
        "}");
}

TEST(test_ir_arreglo) {
    compile(
        "inicio {\n"
        "    entero nums[5] = [10, 20, 30, 40, 50]\n"
        "    entero x = nums[2]\n"
        "}");
}

TEST(test_ir_si_sino) {
    compile(
        "funcion max(entero a, entero b) entero {\n"
        "    si a > b {\n"
        "        devolver a\n"
        "    } sino {\n"
        "        devolver b\n"
        "    }\n"
        "}\n"
        "inicio {}");
}

TEST(test_ir_procedimiento) {
    compile(
        "procedimiento noop(entero x) {\n"
        "    entero y = x + 1\n"
        "}\n"
        "inicio { noop(42) }");
}

TEST(test_ir_enlazar) {
    compile(
        "enlazar vacio imprimir(entero)\n"
        "inicio {}");
}

TEST(test_ir_booleanos) {
    compile(
        "inicio {\n"
        "    booleano a = verdadero\n"
        "    booleano b = falso\n"
        "    booleano c = a y b\n"
        "    booleano d = no a\n"
        "    booleano e = 5 > 3\n"
        "}");
}

TEST(test_ir_fibonacci_completo) {
    compile(
        "funcion fibonacci(entero n) entero {\n"
        "    si n < 2 {\n"
        "        devolver n\n"
        "    }\n"
        "    devolver fibonacci(n - 1) + fibonacci(n - 2)\n"
        "}\n"
        "inicio {\n"
        "    entero r = fibonacci(10)\n"
        "}");
}

TEST(test_ir_decimal) {
    compile(
        "funcion area(decimal base, decimal altura) decimal {\n"
        "    devolver base * altura / 2.0\n"
        "}\n"
        "inicio { decimal r = area(5.0, 3.0) }");
}

TEST(test_ir_texto) {
    compile(
        "enlazar vacio printf(texto)\n"
        "inicio { printf(\"hola\") }");
}

TEST(test_ir_negacion_unaria) {
    compile(
        "funcion neg(entero x) entero {\n"
        "    devolver -x\n"
        "}\n"
        "inicio {}");
}

TEST(test_ir_multiples_funciones) {
    compile(
        "funcion doble(entero x) entero { devolver x * 2 }\n"
        "funcion triple(entero x) entero { devolver x * 3 }\n"
        "procedimiento noop() { entero x = 0 }\n"
        "inicio {\n"
        "    entero a = doble(5)\n"
        "    entero b = triple(3)\n"
        "    noop()\n"
        "}");
}

int main() {
    std::cout << "\n-- Tests del IRGenerator + JIT de KEM --\n\n";
    std::cout << "\n-----\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
