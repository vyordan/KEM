#pragma once

#include <vector>
#include <string>
#include <memory>
#include "kem/Token.hpp"
#include "kem/AST.hpp"
#include "kem/ErrorHandler.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  Parser
//
//  Toma el vector de tokens del Lexer y construye
//  el AST según la gramática EBNF de KEM.
//
//  Técnica:
//  - Recursive Descent para sentencias y declaraciones
//  - Pratt parsing para expresiones (maneja precedencia
//    de operadores sin una función por nivel)
//
//  Recuperación de errores:
//  - Al encontrar un error de sintaxis, reporta el error
//    y avanza ("sincroniza") hasta el próximo punto
//    de recuperación: NEWLINE, RBRACE, o EOF.
//  - Continúa parseando para reportar más errores.
//  - Si hay errores acumulados, lanza al final.
// ─────────────────────────────────────────────
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Punto de entrada — retorna la raíz del AST.
    // Si hubo errores de sintaxis, los reporta todos
    // y lanza KemError con el conteo al final.
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens_;
    int                pos_    = 0;
    int                errors_ = 0;   // contador de errores durante recuperación

    // ── Navegación ──────────────────────────────
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    const Token& advance();
    bool         isAtEnd() const;
    bool         check(TokenType t) const;
    bool         match(TokenType t);

    // Consume el token esperado o registra un error
    const Token& expect(TokenType t, const std::string& msg);

    // Salta NEWLINEs — el parser los ignora en la mayoría de contextos
    void         skipNewlines();

    // ── Recuperación de errores ─────────────────
    // Reporta el error y avanza hasta un punto seguro
    void         syncError(const std::string& msg, int line, int col);
    // Avanza hasta NEWLINE, RBRACE o EOF
    void         synchronize();

    // ── Tipos ───────────────────────────────────
    TypeAnnotation parseType();
    TypeKind       tokenToTypeKind(TokenType t) const;
    bool           isTypeToken(TokenType t) const;

    // ── Declaraciones de nivel superior ─────────
    NodePtr parseTopLevel();
    NodePtr parseFuncDef();
    NodePtr parseProcDef();
    NodePtr parseStructDef();
    NodePtr parseLinkDecl();

    // Parámetros de función/procedimiento
    std::vector<Param> parseParams();
    Param              parseParam();

    // ── Sentencias ───────────────────────────────
    NodePtr parseStmt();
    NodePtr parseVarOrArrayDecl();   // entero x = 5  /  entero nums[3] = [...]
    NodePtr parseIfStmt();
    NodePtr parseWhileStmt();
    NodePtr parseForStmt();          // x = 0 hasta 10 [paso 2] { }
    NodePtr parseReturnStmt();
    NodePtr parseBlock();

    // Sentencia que empieza con un identificador:
    // puede ser asignación, llamada, o inicio de for
    NodePtr parseIdentStmt();

    // ── Expresiones (Pratt) ──────────────────────
    // minPrec controla la precedencia mínima que se
    // puede combinar en este nivel de recursión
    NodePtr parseExpr(int minPrec = 0);
    NodePtr parsePrimary();
    NodePtr parsePostfix(NodePtr left);

    // Tabla de precedencias de operadores binarios
    // Retorna -1 si el token no es un operador binario
    int     binopPrec(const Token& t) const;
    bool    isBinop(const Token& t) const;
    std::string binopLexeme(const Token& t) const;
};

} // namespace kem
