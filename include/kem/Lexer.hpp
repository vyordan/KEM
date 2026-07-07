#pragma once

#include <string>
#include <vector>
#include "kem/Token.hpp"
#include "kem/LangConfig.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  Lexer
//
//  Lee el código fuente carácter por carácter y
//  produce un vector de Tokens.
//
//  Reglas importantes:
//  - No tiene conocimiento del idioma — consulta LangConfig
//  - Emite NEWLINE como token; el Parser decide si es fin de sentencia
//  - Cada Token lleva línea y columna para mensajes de error
//  - Los comentarios se descartan — nunca producen tokens
//
//  Continuación de línea (sin punto y coma):
//  El Lexer emite NEWLINE normalmente. El Parser lo descarta cuando:
//    1. La línea anterior terminó en operador binario
//    2. Hay paréntesis o corchetes sin cerrar
//    3. La línea anterior terminó en '{'
// ─────────────────────────────────────────────
class Lexer {
public:
    // src:    contenido completo del archivo .kem (se copia internamente)
    // config: instancia ya cargada de LangConfig (vive más que el Lexer
    //         en el flujo normal del CLI, por eso sí se referencia)
    explicit Lexer(std::string src, const LangConfig& config);

    // Tokeniza todo el fuente de una vez.
    // Lanza KemError(Phase::LEXER, ...) ante cualquier carácter inválido.
    std::vector<Token> tokenize();

private:
    // NOTA: src_ se guarda como COPIA (no referencia) a propósito.
    // Si fuera `const std::string&`, pasar un literal de C-string como
    // `Lexer(\"texto\", cfg)` crea un std::string temporal que se destruye
    // al final de la expresión, dejando una referencia colgante (dangling
    // reference) — el Lexer leería memoria liberada y produciría bytes
    // basura. Copiar el string evita esta clase de bug por completo.
    std::string         src_;
    const LangConfig&   config_;

    int pos_  = 0;   // posición actual en src_
    int line_ = 1;   // línea actual (1-indexada)
    int col_  = 1;   // columna actual (1-indexada)

    // ── Navegación ─────────────────────────────
    char current() const;          // carácter en pos_
    char peek(int offset = 1) const; // mira adelante sin avanzar
    char advance();                // consume y retorna current(), avanza pos_
    bool isAtEnd() const;

    // ── Builders de tokens ─────────────────────
    Token makeToken(TokenType type, const std::string& lexeme) const;
    Token makeToken(TokenType type, char c) const;

    // ── Lectura de cada tipo de token ──────────
    Token readNumber();     // INTEGER_LIT o FLOAT_LIT
    Token readString();     // STRING_LIT (delimitado por "")
    Token readIdentOrKw();  // IDENT o keyword (consulta LangConfig)
    Token readSymbol();     // operadores y delimitadores
    Token readNewline();    // NEWLINE (puede consumir múltiples '\n' consecutivos)

    // ── Comentarios (no producen tokens) ───────
    void skipLineComment();    // // hasta fin de línea
    void skipBlockComment();   // /* ... */
    void skipKemLineComment(); // comentario hasta fin de línea
    void skipKemBlockComment();// comentario{ ... }

    // ── Utilidades ─────────────────────────────
    void skipWhitespaceNoNewline(); // consume espacios y tabs, NO newlines
    bool isDigit(char c)    const;
    bool isAlpha(char c)    const;
    bool isAlphaNum(char c) const;

    // ── Estado de columna ──────────────────────
    // Guardamos col_ al inicio del token para que el Token tenga
    // la columna del PRIMER carácter, no del último
    int tokenStartCol_ = 1;
    int tokenStartLine_= 1;
    void markTokenStart(); // llama antes de empezar a leer un token
};

} // namespace kem
