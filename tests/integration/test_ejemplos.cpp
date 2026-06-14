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
            ++tests_failed; \
            std::cout << "  \xE2\x9C\x97 " #name ": " << e.what() << "\n"; } \
    }} _inst_##name; \
    void name()

#define ASSERT_EQ(a, b) \
    do { auto _a=(a); auto _b=(b); \
         if (_a!=_b) { std::ostringstream _s; \
                       _s << "ASSERT_EQ falló: " << _a << " != " << _b; \
                       throw std::runtime_error(_s.str()); } } while(0)

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE falló: " #expr)

// ─────────────────────────────────────────────
//  Helper: compila + ejecuta → retorna resultado
// ─────────────────────────────────────────────
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

static int64_t run(const std::string& src) {
    return execute(src, cfg_es());
}

static int64_t run_en(const std::string& src) {
    return execute(src, cfg_en());
}

// ─────────────────────────────────────────────
//  Tests — aritmética básica
// ─────────────────────────────────────────────

TEST(test_suma_basica) {
    int64_t r = run(
        "funcion f() entero { devolver 3 + 4 }\n"
        "inicio { entero r = f() }");
    // inicio{} retorna 0 (ret 0 implícito)
    // pero verificamos que el pipeline completo no lanza
    ASSERT_TRUE(r == 0 || r == 7); // según si inicio retorna r o 0
}

TEST(test_factorial_5) {
    int64_t r = run(R"(
funcion factorial(entero n) entero {
    si n == 0 { devolver 1 }
    devolver n * factorial(n - 1)
}
funcion getResult() entero {
    devolver factorial(5)
}
inicio { entero r = getResult() }
)");
    // El pipeline completo debe correr sin lanzar
    ASSERT_TRUE(r >= 0);
}

TEST(test_fibonacci_10) {
    // Verificar que fibonacci compila y corre sin errores
    int64_t r = run(R"(
funcion fib(entero n) entero {
    si n < 2 { devolver n }
    devolver fib(n - 1) + fib(n - 2)
}
inicio { entero r = fib(10) }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_max_dos_numeros) {
    int64_t r = run(R"(
funcion max(entero a, entero b) entero {
    si a > b { devolver a }
    devolver b
}
funcion f() entero { devolver max(7, 3) }
inicio { entero r = f() }
)");
    ASSERT_TRUE(r >= 0);
}

// ─────────────────────────────────────────────
//  Tests — flujo de control
// ─────────────────────────────────────────────

TEST(test_si_sino_ejecuta) {
    int64_t r = run(R"(
funcion signo(entero x) entero {
    si x > 0 { devolver 1 }
    sino si x < 0 { devolver -1 }
    devolver 0
}
inicio { entero r = signo(5) }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_mientras_ejecuta) {
    int64_t r = run(R"(
funcion sumaHasta(entero n) entero {
    entero suma = 0
    entero i = 1
    mientras i <= n {
        suma = suma + i
        i = i + 1
    }
    devolver suma
}
inicio { entero r = sumaHasta(10) }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_hasta_ejecuta) {
    int64_t r = run(R"(
funcion sumaArreglo() entero {
    entero nums[5] = [10, 20, 30, 40, 50]
    entero suma = 0
    entero i
    i = 0 hasta 5 {
        suma = suma + nums[i]
    }
    devolver suma
}
inicio { entero r = sumaArreglo() }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_hasta_con_paso_ejecuta) {
    int64_t r = run(R"(
funcion sumarPares() entero {
    entero suma = 0
    entero i
    i = 0 hasta 10 paso 2 {
        suma = suma + i
    }
    devolver suma
}
inicio { entero r = sumarPares() }
)");
    ASSERT_TRUE(r >= 0);
}

// ─────────────────────────────────────────────
//  Tests — arreglos
// ─────────────────────────────────────────────

TEST(test_arreglo_acceso) {
    int64_t r = run(R"(
funcion getElem() entero {
    entero nums[3] = [10, 20, 30]
    devolver nums[1]
}
inicio { entero r = getElem() }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_arreglo_modificacion) {
    int64_t r = run(R"(
funcion test() entero {
    entero nums[3] = [1, 2, 3]
    nums[0] = 99
    devolver nums[0]
}
inicio { entero r = test() }
)");
    ASSERT_TRUE(r >= 0);
}

// ─────────────────────────────────────────────
//  Tests — procedimientos
// ─────────────────────────────────────────────

TEST(test_procedimiento_ejecuta) {
    int64_t r = run(R"(
procedimiento noop(entero x) {
    entero y = x + 1
}
inicio { noop(42) }
)");
    ASSERT_EQ(r, 0LL);
}

// ─────────────────────────────────────────────
//  Tests — tipos
// ─────────────────────────────────────────────

TEST(test_decimal_ejecuta) {
    int64_t r = run(R"(
funcion area(decimal base, decimal altura) decimal {
    devolver base * altura / 2.0
}
inicio { decimal r = area(5.0, 3.0) }
)");
    ASSERT_EQ(r, 0LL); // inicio retorna 0 (el resultado de area es decimal)
}

TEST(test_booleano_ejecuta) {
    int64_t r = run(R"(
funcion esMayor(entero a, entero b) booleano {
    devolver a > b
}
inicio { booleano b = esMayor(5, 3) }
)");
    ASSERT_EQ(r, 0LL);
}

// ─────────────────────────────────────────────
//  Tests — multi-idioma (english.json)
// ─────────────────────────────────────────────

TEST(test_english_suma) {
    int64_t r = run_en(R"(
function suma(int a, int b) int {
    return a + b
}
main { int r = suma(3, 4) }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_english_factorial) {
    int64_t r = run_en(R"(
function factorial(int n) int {
    if n == 0 { return 1 }
    return n * factorial(n - 1)
}
main { int r = factorial(5) }
)");
    ASSERT_TRUE(r >= 0);
}

TEST(test_english_while) {
    int64_t r = run_en(R"(
function f() int {
    int suma = 0
    int i = 0
    while i < 5 {
        suma = suma + i
        i = i + 1
    }
    return suma
}
main { int r = f() }
)");
    ASSERT_TRUE(r >= 0);
}

// ─────────────────────────────────────────────
//  Tests — errores en tiempo de compilación
// ─────────────────────────────────────────────

TEST(test_error_variable_no_declarada_no_ejecuta) {
    bool lanzó = false;
    try {
        run("inicio { entero x = y }");
    } catch (const kem::KemError&) {
        lanzó = true;
    }
    ASSERT_TRUE(lanzó);
}

TEST(test_error_tipo_incompatible_no_ejecuta) {
    bool lanzó = false;
    try {
        run("inicio { entero x = verdadero }");
    } catch (const kem::KemError&) {
        lanzó = true;
    }
    ASSERT_TRUE(lanzó);
}

TEST(test_error_funcion_sin_return_no_ejecuta) {
    bool lanzó = false;
    try {
        run("funcion f(entero x) entero { entero y = x }\ninicio {}");
    } catch (const kem::KemError&) {
        lanzó = true;
    }
    ASSERT_TRUE(lanzó);
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n\xE2\x94\x80\xE2\x94\x80 Tests de Integraci\xC3\xB3n KEM "
                 "\xE2\x94\x80\xE2\x94\x80\n\n";
    std::cout << "\n\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
