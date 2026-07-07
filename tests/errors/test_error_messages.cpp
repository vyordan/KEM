#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "kem/LangConfig.hpp"
#include "kem/ErrorMessages.hpp"
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
        try { name(); ++tests_passed; std::cout << "  OK " #name "\n"; } \
        catch (const std::exception& e) { \
            ++tests_failed; std::cout << "  FAIL " #name ": " << e.what() << "\n"; } \
    }} _inst_##name; \
    void name()

#define ASSERT_EQ(a, b) \
    do { auto _a=(a); auto _b=(b); \
         if (_a!=_b) { std::ostringstream _s; \
                       _s << "ASSERT_EQ: '" << _a << "' != '" << _b << "'"; \
                       throw std::runtime_error(_s.str()); } } while(0)

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE fallo: " #expr)

#define ASSERT_CONTAINS(haystack, needle) \
    if ((haystack).find(needle) == std::string::npos) \
        throw std::runtime_error("ASSERT_CONTAINS fallo: '" + \
            std::string(needle) + "' no esta en '" + (haystack) + "'")

// ─────────────────────────────────────────────
//  Tests — ErrorMessages aislado (sin pipeline completo)
// ─────────────────────────────────────────────

TEST(test_carga_espanol_ok) {
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("SEM_VAR_NO_DECLARADA", {"x"});
    ASSERT_EQ(r, std::string("Variable 'x' no declarada"));
}

TEST(test_carga_english_ok) {
    kem::ErrorMessages msgs("langs/errors/english.json");
    std::string r = msgs.format("SEM_VAR_NO_DECLARADA", {"x"});
    ASSERT_EQ(r, std::string("Variable 'x' is not declared"));
}

TEST(test_placeholder_unico) {
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("LEX_CHAR_INESPERADO", {"@"});
    ASSERT_EQ(r, std::string("Carácter inesperado '@'"));
}

TEST(test_placeholders_multiples) {
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("SEM_ARGS_CANTIDAD", {"suma", "2", "3"});
    ASSERT_EQ(r, std::string("'suma' espera 2 argumento(s), recibió 3"));
}

TEST(test_placeholders_multiples_ingles) {
    kem::ErrorMessages msgs("langs/errors/english.json");
    std::string r = msgs.format("SEM_ARGS_CANTIDAD", {"suma", "2", "3"});
    ASSERT_EQ(r, std::string("'suma' expects 2 argument(s), got 3"));
}

TEST(test_codigo_repetido_en_mensaje) {
    // Mismo placeholder usado mas de una vez no esta soportado por diseño
    // (cada {i} se sustituye una sola vez por args[i]) — verificamos que
    // al menos no rompe con placeholders consecutivos distintos
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("SEM_ARGS_TIPO", {"1", "f", "entero", "texto"});
    ASSERT_EQ(r, std::string(
        "Argumento 1 de 'f': se esperaba 'entero' pero se pasó 'texto'"));
}

TEST(test_codigo_inexistente_fallback) {
    // Instancia cargada pero con un codigo que no existe en el archivo
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("CODIGO_QUE_NO_EXISTE", {"arg1"});
    ASSERT_EQ(r, std::string("[CODIGO_QUE_NO_EXISTE] arg1"));
}

TEST(test_instancia_nula_fallback) {
    // ErrorMessages() sin cargar ningun archivo — degradación controlada
    kem::ErrorMessages msgs;
    std::string r = msgs.format("SEM_VAR_NO_DECLARADA", {"x"});
    ASSERT_EQ(r, std::string("[SEM_VAR_NO_DECLARADA] x"));
}

TEST(test_sin_argumentos) {
    kem::ErrorMessages msgs("langs/errors/espanol.json");
    std::string r = msgs.format("SEM_SI_CONDICION_BOOLEANA");
    ASSERT_EQ(r, std::string("La condición del 'si' debe ser booleana"));
}

TEST(test_archivo_inexistente_lanza) {
    bool lanzo = false;
    try {
        kem::ErrorMessages msgs("langs/errors/no_existe.json");
    } catch (const kem::KemError&) {
        lanzo = true;
    }
    ASSERT_TRUE(lanzo);
}

// ─────────────────────────────────────────────
//  Tests — integración con el pipeline completo
//  Verifican que el mensaje de error end-to-end sale
//  en el idioma correcto según --lang
// ─────────────────────────────────────────────

// El SemanticAnalyzer imprime el detalle de cada error a std::cerr
// inmediatamente cuando ocurre, y al final lanza un KemError-resumen
// ("El programa tiene N errores"). Para verificar el TEXTO localizado
// del error puntual, capturamos std::cerr durante la compilación.
static std::string compilarConIdiomaCapturandoCerr(
    const std::string& src,
    const std::string& lang_path,
    const std::string& errors_path,
    bool& lanzo)
{
    kem::LangConfig cfg(lang_path);
    kem::setErrorMessages(kem::ErrorMessages(errors_path));

    std::ostringstream captured;
    std::streambuf* old_cerr = std::cerr.rdbuf(captured.rdbuf());

    lanzo = false;
    try {
        kem::Lexer lexer(src, cfg);
        auto tokens = lexer.tokenize();
        kem::Parser parser(std::move(tokens));
        auto prog = parser.parse();
        kem::SemanticAnalyzer sem;
        sem.analyze(*prog);
    } catch (const kem::KemError&) {
        lanzo = true;
    }

    std::cerr.rdbuf(old_cerr);
    return captured.str();
}

TEST(test_error_semantico_en_espanol) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "inicio { entero x = z }", "langs/espanol.json", "langs/errors/espanol.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "no declarada");
}

TEST(test_error_semantico_en_ingles) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "main { int x = z }", "langs/english.json", "langs/errors/english.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "is not declared");
}

TEST(test_error_funcion_sin_retorno_espanol) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "funcion f(entero x) entero { entero y = x }\ninicio {}", "langs/espanol.json", "langs/errors/espanol.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "no siempre retorna un valor");
}

TEST(test_error_funcion_sin_retorno_ingles) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "function f(int x) int { int y = x }\nmain {}", "langs/english.json", "langs/errors/english.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "does not always return a value");
}

TEST(test_error_args_cantidad_espanol) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "funcion suma(entero a, entero b) entero { devolver a + b }\n"
        "inicio { entero r = suma(1) }",
        "langs/espanol.json", "langs/errors/espanol.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "argumento(s)");
}

TEST(test_error_args_cantidad_ingles) {
    bool lanzo = false;
    std::string captured = compilarConIdiomaCapturandoCerr(
        "function suma(int a, int b) int { return a + b }\n"
        "main { int r = suma(1) }",
        "langs/english.json", "langs/errors/english.json", lanzo);
    ASSERT_TRUE(lanzo);
    ASSERT_CONTAINS(captured, "argument(s)");
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n-- Tests del sistema de mensajes de error (ErrorMessages) --\n\n";
    std::cout << "\n-----\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
