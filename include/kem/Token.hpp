#pragma once

#include <string>

namespace kem {

// ─────────────────────────────────────────────
//  TokenType
//  Enum interno del compilador — completamente
//  independiente del idioma del archivo fuente.
//  LangConfig mapea strings del JSON a estos valores.
// ─────────────────────────────────────────────
enum class TokenType {

    // ── Literales ──────────────────────────────
    INTEGER_LIT,     // 42
    FLOAT_LIT,       // 3.14
    STRING_LIT,      // "hola"
    IDENT,           // nombre de variable o función

    // ── Tipos de dato ──────────────────────────
    KW_ENTERO,       // entero
    KW_DECIMAL,      // decimal
    KW_TEXTO,        // texto
    KW_BOOLEANO,     // booleano

    // ── Funciones y bloques ────────────────────
    KW_FUNCION,      // funcion
    KW_PROC,         // procedimiento
    KW_DEVOLVER,     // devolver
    KW_INICIO,       // inicio  (punto de entrada, equivalente a main)

    // ── Flujo de control ───────────────────────
    KW_SI,           // si
    KW_SINO,         // sino
    KW_MIENTRAS,     // mientras
    KW_HASTA,        // hasta
    KW_PASO,         // paso

    // ── Lógica ─────────────────────────────────
    KW_VERDADERO,    // verdadero
    KW_FALSO,        // falso
    KW_Y,            // y
    KW_O,            // o
    KW_NO,           // no

    // ── Tipos compuestos ───────────────────────
    KW_ESTRUCTURA,   // estructura

    // ── Gestión de memoria / interop ───────────
    KW_REF,          // referencia
    KW_ENLAZAR,      // enlazar

    // ── Operadores aritméticos ─────────────────
    PLUS,            // +
    MINUS,           // -
    STAR,            // *
    SLASH,           // /
    PERCENT,         // %

    // ── Operadores relacionales ────────────────
    EQ,              // =
    EQEQ,            // ==
    NEQ,             // !=
    LT,              // <
    GT,              // >
    LTE,             // <=
    GTE,             // >=

    // ── Delimitadores ──────────────────────────
    LPAREN,          // (
    RPAREN,          // )
    LBRACE,          // {
    RBRACE,          // }
    LBRACKET,        // [
    RBRACKET,        // ]
    COMMA,           // ,
    DOT,             // .

    // ── Control de línea ───────────────────────
    // El Parser decide si este token termina una sentencia
    // o si la línea continúa (termina en operador, paréntesis abierto, etc.)
    NEWLINE,

    // ── Fin de archivo ─────────────────────────
    EOF_TOK,

    // ── Error interno (nunca debería llegar al Parser) ──
    UNKNOWN
};

// ─────────────────────────────────────────────
//  Token
//  Unidad mínima producida por el Lexer.
//  Viaja a través de todo el pipeline hasta el Parser.
// ─────────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string lexeme;  // texto original tal como aparece en el fuente
    int         line;    // 1-indexado
    int         col;     // 1-indexado — columna del primer carácter

    Token(TokenType t, std::string lex, int ln, int cl)
        : type(t), lexeme(std::move(lex)), line(ln), col(cl) {}

    // Utilidad para debugging y tests
    bool is(TokenType t)  const { return type == t; }
    bool isNot(TokenType t) const { return type != t; }
};

// Convierte un TokenType a string legible — útil para mensajes de error
const char* tokenTypeName(TokenType t);

} // namespace kem
