#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "kem/Token.hpp"
#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/ErrorHandler.hpp"

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Macro genérica: funciona con int, string, size_t, etc.
#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            std::ostringstream _ss; \
            _ss << "ASSERT_EQ falló: '" << _a << "' != '" << _b << "'"; \
            throw std::runtime_error(_ss.str()); \
        } \
    } while(0)

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE falló: " #expr)

#define ASSERT_THROWS(expr) \
    { bool _threw = false; \
      try { expr; } catch (...) { _threw = true; } \
      if (!_threw) throw std::runtime_error("ASSERT_THROWS falló: se esperaba una excepción"); }

#define TEST(name) \
    void name(); \
    struct _reg_##name { _reg_##name() { \
        ++tests_run; \
        try { name(); ++tests_passed; std::cout << "  ✓ " #name "\n"; } \
        catch (const std::exception& e) { ++tests_failed; std::cout << "  ✗ " #name ": " << e.what() << "\n"; } \
    }} _inst_##name; \
    void name()

static kem::LangConfig& getConfig() {
    static kem::LangConfig config("langs/espanol.json");
    return config;
}

static std::vector<kem::Token> lex(const std::string& src) {
    kem::Lexer lexer(src, getConfig());
    return lexer.tokenize();
}

TEST(test_keywords_espanol) {
    auto tokens = lex("funcion procedimiento devolver inicio");
    ASSERT_EQ((int)tokens.size(), 5);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::KW_FUNCION));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::KW_PROC));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::KW_DEVOLVER));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::KW_INICIO));
}

TEST(test_tipos) {
    auto tokens = lex("entero decimal texto booleano");
    ASSERT_EQ((int)tokens.size(), 5);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::KW_ENTERO));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::KW_DECIMAL));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::KW_TEXTO));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::KW_BOOLEANO));
}

TEST(test_flujo_de_control) {
    auto tokens = lex("si sino mientras hasta paso");
    ASSERT_EQ((int)tokens.size(), 6);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::KW_SI));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::KW_SINO));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::KW_MIENTRAS));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::KW_HASTA));
    ASSERT_TRUE(tokens[4].is(kem::TokenType::KW_PASO));
}

TEST(test_logica) {
    auto tokens = lex("verdadero falso y o no");
    ASSERT_EQ((int)tokens.size(), 6);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::KW_VERDADERO));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::KW_FALSO));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::KW_Y));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::KW_O));
    ASSERT_TRUE(tokens[4].is(kem::TokenType::KW_NO));
}

TEST(test_integer_literal) {
    auto tokens = lex("42 0 100");
    ASSERT_EQ((int)tokens.size(), 4);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::INTEGER_LIT));
    ASSERT_EQ(tokens[0].lexeme, std::string("42"));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::INTEGER_LIT));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::INTEGER_LIT));
}

TEST(test_float_literal) {
    auto tokens = lex("3.14 0.5 100.0");
    ASSERT_EQ((int)tokens.size(), 4);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::FLOAT_LIT));
    ASSERT_EQ(tokens[0].lexeme, std::string("3.14"));
}

TEST(test_string_literal) {
    auto tokens = lex("\"hola mundo\"");
    ASSERT_EQ((int)tokens.size(), 2);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::STRING_LIT));
    ASSERT_EQ(tokens[0].lexeme, std::string("hola mundo"));
}

TEST(test_identificador) {
    auto tokens = lex("miVariable _interno variable123");
    ASSERT_EQ((int)tokens.size(), 4);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::IDENT));
    ASSERT_EQ(tokens[0].lexeme, std::string("miVariable"));
}

TEST(test_operadores_aritmeticos) {
    auto tokens = lex("+ - * / %");
    ASSERT_EQ((int)tokens.size(), 6);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::PLUS));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::MINUS));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::STAR));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::SLASH));
    ASSERT_TRUE(tokens[4].is(kem::TokenType::PERCENT));
}

TEST(test_operadores_relacionales) {
    auto tokens = lex("== != < > <= >=");
    ASSERT_EQ((int)tokens.size(), 7);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::EQEQ));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::NEQ));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::LT));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::GT));
    ASSERT_TRUE(tokens[4].is(kem::TokenType::LTE));
    ASSERT_TRUE(tokens[5].is(kem::TokenType::GTE));
}

TEST(test_asignacion) {
    auto tokens = lex("x = 5");
    ASSERT_EQ((int)tokens.size(), 4);
    ASSERT_TRUE(tokens[1].is(kem::TokenType::EQ));
}

TEST(test_delimitadores) {
    auto tokens = lex("( ) { } [ ] , .");
    ASSERT_EQ((int)tokens.size(), 9);
    ASSERT_TRUE(tokens[0].is(kem::TokenType::LPAREN));
    ASSERT_TRUE(tokens[1].is(kem::TokenType::RPAREN));
    ASSERT_TRUE(tokens[2].is(kem::TokenType::LBRACE));
    ASSERT_TRUE(tokens[3].is(kem::TokenType::RBRACE));
    ASSERT_TRUE(tokens[4].is(kem::TokenType::LBRACKET));
    ASSERT_TRUE(tokens[5].is(kem::TokenType::RBRACKET));
    ASSERT_TRUE(tokens[6].is(kem::TokenType::COMMA));
    ASSERT_TRUE(tokens[7].is(kem::TokenType::DOT));
}

TEST(test_newline_emitido) {
    auto tokens = lex("entero x\nentero y");
    bool found_newline = false;
    for (const auto& t : tokens) {
        if (t.is(kem::TokenType::NEWLINE)) { found_newline = true; break; }
    }
    ASSERT_TRUE(found_newline);
}

TEST(test_comentario_linea_doble_slash) {
    auto tokens = lex("entero x // esto es un comentario\nentero y");
    for (const auto& t : tokens) {
        ASSERT_TRUE(!t.is(kem::TokenType::UNKNOWN));
    }
    ASSERT_EQ((int)tokens.size(), 6);
}

TEST(test_comentario_bloque) {
    auto tokens = lex("entero x /* comentario de bloque */ entero y");
    ASSERT_EQ((int)tokens.size(), 5);
}

TEST(test_comentario_kem_linea) {
    auto tokens = lex("entero x comentario esto es ignorado\nentero y");
    ASSERT_EQ((int)tokens.size(), 6);
}

TEST(test_comentario_kem_bloque) {
    auto tokens = lex("entero x comentario{ linea uno\nlinea dos } entero y");
    ASSERT_EQ((int)tokens.size(), 5);
}

TEST(test_comentario_no_confunde_identificador_similar) {
    // "comentarioX" es un identificador válido, no debe disparar el comentario
    auto tokens = lex("entero comentarioX = 5");
    ASSERT_EQ((int)tokens.size(), 5); // entero, comentarioX, =, 5, EOF
    ASSERT_TRUE(tokens[1].is(kem::TokenType::IDENT));
    ASSERT_EQ(tokens[1].lexeme, std::string("comentarioX"));
}

TEST(test_comentario_palabra_configurable_en_ingles) {
    // El archivo english.json define "comment" como palabra de comentario
    kem::LangConfig cfg_en("langs/english.json");
    kem::Lexer lexer("int x comment this is ignored\nint y", cfg_en);
    auto tokens = lexer.tokenize();
    ASSERT_EQ((int)tokens.size(), 6); // int, x, NEWLINE, int, y, EOF
}

TEST(test_comentario_bloque_palabra_configurable_en_ingles) {
    kem::LangConfig cfg_en("langs/english.json");
    kem::Lexer lexer("int x comment{ ignored\nmultiline } int y", cfg_en);
    auto tokens = lexer.tokenize();
    ASSERT_EQ((int)tokens.size(), 5); // int, x, int, y, EOF
}

TEST(test_linea_columna) {
    auto tokens = lex("funcion\nfibonacci");
    ASSERT_EQ(tokens[0].line, 1);
    ASSERT_EQ(tokens[0].col,  1);
    ASSERT_EQ(tokens[2].line, 2);
    ASSERT_EQ(tokens[2].col,  1);
}

TEST(test_error_caracter_invalido) {
    ASSERT_THROWS(lex("entero x = @5"));
}

TEST(test_string_sin_cerrar) {
    ASSERT_THROWS(lex("entero x = \"sin cerrar"));
}

TEST(test_programa_completo) {
    std::string prog = R"(
funcion fibonacci(entero n) entero {
    si n < 2 {
        devolver n
    }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}
inicio {
    entero resultado = fibonacci(10)
}
)";
    auto tokens = lex(prog);
    ASSERT_TRUE(!tokens.empty());
    ASSERT_TRUE(tokens.back().is(kem::TokenType::EOF_TOK));
}

int main() {
    std::cout << "\n── Tests del Lexer de KEM ──────────────────\n\n";
    std::cout << "\n────────────────────────────────────────────\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
