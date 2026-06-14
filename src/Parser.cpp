#include "kem/Parser.hpp"
#include <sstream>
#include <iostream>

namespace kem {

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {}

// ─────────────────────────────────────────────
//  Navegación
// ─────────────────────────────────────────────
const Token& Parser::current() const {
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx >= (int)tokens_.size()) return tokens_.back(); // EOF
    return tokens_[idx];
}

const Token& Parser::advance() {
    const Token& t = tokens_[pos_];
    if (!isAtEnd()) ++pos_;
    return t;
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::EOF_TOK;
}

bool Parser::check(TokenType t) const {
    return current().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    syncError(msg, current().line, current().col);
    return current(); // nunca se alcanza tras syncError
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) advance();
}

// ─────────────────────────────────────────────
//  Recuperación de errores
// ─────────────────────────────────────────────
void Parser::syncError(const std::string& msg, int line, int col) {
    ++errors_;
    // Imprimir el error inmediatamente para acumular varios
    std::cerr << KemError(Phase::PARSER, msg, line, col).what() << "\n";
    synchronize();
    // Lanzar para salir del frame de parseo actual y volver
    // al nivel que maneja la recuperación
    throw KemError(Phase::PARSER, msg, line, col);
}

void Parser::synchronize() {
    // Avanzar hasta un punto seguro desde donde continuar
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); return; }
        if (check(TokenType::RBRACE))  { return; }
        advance();
    }
}

// ─────────────────────────────────────────────
//  Tipos
// ─────────────────────────────────────────────
bool Parser::isTypeToken(TokenType t) const {
    return t == TokenType::KW_ENTERO  ||
           t == TokenType::KW_DECIMAL ||
           t == TokenType::KW_TEXTO   ||
           t == TokenType::KW_BOOLEANO;
}

TypeKind Parser::tokenToTypeKind(TokenType t) const {
    switch (t) {
        case TokenType::KW_ENTERO:   return TypeKind::ENTERO;
        case TokenType::KW_DECIMAL:  return TypeKind::DECIMAL;
        case TokenType::KW_TEXTO:    return TypeKind::TEXTO;
        case TokenType::KW_BOOLEANO: return TypeKind::BOOLEANO;
        default:                     return TypeKind::UNKNOWN;
    }
}

TypeAnnotation Parser::parseType() {
    TypeAnnotation ta;

    // Tipo void especial para enlazar
    if (check(TokenType::IDENT) && current().lexeme == "vacio") {
        advance();
        ta.kind = TypeKind::VOID;
        return ta;
    }

    if (!isTypeToken(current().type)) {
        syncError("Se esperaba un tipo (entero, decimal, texto, booleano)",
                  current().line, current().col);
    }

    ta.kind = tokenToTypeKind(current().type);
    advance();

    // ¿Es un arreglo? tipo[N]
    if (check(TokenType::LBRACKET)) {
        advance(); // consume '['
        if (!check(TokenType::INTEGER_LIT)) {
            syncError("Se esperaba el tamaño del arreglo (un entero literal)",
                      current().line, current().col);
        }
        ta.is_array   = true;
        ta.array_size = std::stoi(current().lexeme);
        advance(); // consume el número
        expect(TokenType::RBRACKET, "Se esperaba ']' después del tamaño del arreglo");
    }

    return ta;
}

// ─────────────────────────────────────────────
//  Punto de entrada principal
// ─────────────────────────────────────────────
std::unique_ptr<Program> Parser::parse() {
    skipNewlines();

    std::vector<NodePtr> decls;
    NodePtr main_block;

    while (!isAtEnd()) {
        skipNewlines();
        if (isAtEnd()) break;

        try {
            // inicio { } — puede aparecer en cualquier posición
            if (check(TokenType::KW_INICIO)) {
                int ln = current().line, cl = current().col;
                advance(); // consume 'inicio'
                skipNewlines();
                main_block = parseBlock();
                // Anotar línea del inicio
                if (main_block) { main_block->line = ln; main_block->col = cl; }
            } else {
                auto decl = parseTopLevel();
                if (decl) decls.push_back(std::move(decl));
            }
        } catch (const KemError&) {
            // Error ya reportado en syncError — seguir parseando
            skipNewlines();
        }
    }

    if (errors_ > 0) {
        std::ostringstream ss;
        ss << "El programa tiene " << errors_
           << " error(es) de sintaxis. Corregílos antes de continuar.";
        throw KemError(Phase::PARSER, ss.str());
    }

    return std::make_unique<Program>(std::move(decls), std::move(main_block));
}

// ─────────────────────────────────────────────
//  Declaraciones de nivel superior
// ─────────────────────────────────────────────
NodePtr Parser::parseTopLevel() {
    if (check(TokenType::KW_FUNCION))    return parseFuncDef();
    if (check(TokenType::KW_PROC))       return parseProcDef();
    if (check(TokenType::KW_ESTRUCTURA)) return parseStructDef();
    if (check(TokenType::KW_ENLAZAR))    return parseLinkDecl();

    syncError("Se esperaba 'funcion', 'procedimiento', 'estructura', 'enlazar' o 'inicio'",
              current().line, current().col);
    return nullptr;
}

// ─────────────────────────────────────────────
//  funcion nombre(params) tipo { body }
// ─────────────────────────────────────────────
NodePtr Parser::parseFuncDef() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'funcion'

    if (!check(TokenType::IDENT)) {
        syncError("Se esperaba el nombre de la función", current().line, current().col);
    }
    std::string name = current().lexeme;
    advance();

    expect(TokenType::LPAREN, "Se esperaba '(' después del nombre de la función");
    auto params = parseParams();
    expect(TokenType::RPAREN, "Se esperaba ')' para cerrar los parámetros");

    // tipo de retorno obligatorio en funcion
    TypeAnnotation ret = parseType();

    skipNewlines();
    auto body = parseBlock();

    return std::make_unique<FuncDef>(name, std::move(params), ret,
                                     std::move(body), ln, cl);
}

// ─────────────────────────────────────────────
//  procedimiento nombre(params) { body }
// ─────────────────────────────────────────────
NodePtr Parser::parseProcDef() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'procedimiento'

    if (!check(TokenType::IDENT)) {
        syncError("Se esperaba el nombre del procedimiento", current().line, current().col);
    }
    std::string name = current().lexeme;
    advance();

    expect(TokenType::LPAREN, "Se esperaba '(' después del nombre del procedimiento");
    auto params = parseParams();
    expect(TokenType::RPAREN, "Se esperaba ')' para cerrar los parámetros");

    skipNewlines();
    auto body = parseBlock();

    return std::make_unique<ProcDef>(name, std::move(params), std::move(body), ln, cl);
}

// ─────────────────────────────────────────────
//  Parámetros
// ─────────────────────────────────────────────
std::vector<Param> Parser::parseParams() {
    std::vector<Param> params;
    skipNewlines();

    if (check(TokenType::RPAREN)) return params; // lista vacía

    params.push_back(parseParam());

    while (match(TokenType::COMMA)) {
        skipNewlines();
        params.push_back(parseParam());
    }

    return params;
}

Param Parser::parseParam() {
    Param p;
    p.line = current().line;
    p.col  = current().col;

    // ¿es por referencia?
    if (check(TokenType::KW_REF)) {
        p.is_ref = true;
        advance();
    }

    // Tipo primitivo (entero, decimal, texto, booleano)
    if (isTypeToken(current().type)) {
        p.type = parseType();
    }
    // Tipo de usuario (nombre de estructura) — el semántico lo valida
    else if (check(TokenType::IDENT)) {
        p.type_name = current().lexeme;
        p.type.kind = TypeKind::UNKNOWN; // marcado como tipo de usuario
        advance();
    } else {
        syncError("Se esperaba un tipo para el parámetro", current().line, current().col);
    }

    // En contexto de parámetro, cualquier token no vacío es un nombre válido
    // (permite parámetros llamados "x", "y", etc.)
    if (!isAtEnd() && !check(TokenType::COMMA) && !check(TokenType::RPAREN)) {
        p.name = current().lexeme;
        advance();
    } else {
        syncError("Se esperaba el nombre del parámetro", current().line, current().col);
    }

    return p;
}

// ─────────────────────────────────────────────
//  estructura Nombre { campos }
// ─────────────────────────────────────────────
NodePtr Parser::parseStructDef() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'estructura'

    if (!check(TokenType::IDENT)) {
        syncError("Se esperaba el nombre de la estructura", current().line, current().col);
    }
    std::string name = current().lexeme;
    advance();

    skipNewlines();
    expect(TokenType::LBRACE, "Se esperaba '{' para abrir la estructura");
    skipNewlines();

    std::vector<StructField> fields;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RBRACE)) break;

        StructField f;
        f.line = current().line;
        f.col  = current().col;
        f.type = parseType();

        // En contexto de campo de estructura, cualquier token no-tipo es un nombre válido
        // Esto permite campos como "x", "y", "z" aunque colisionen con keywords
        if (check(TokenType::IDENT) || current().type >= TokenType::KW_ENTERO) {
            f.name = current().lexeme;
            advance();
        } else {
            syncError("Se esperaba el nombre del campo", current().line, current().col);
        }
        fields.push_back(std::move(f));

        skipNewlines();
    }

    expect(TokenType::RBRACE, "Se esperaba '}' para cerrar la estructura");
    return std::make_unique<StructDef>(name, std::move(fields), ln, cl);
}

// ─────────────────────────────────────────────
//  enlazar tipo nombre(tipo1, tipo2, ...)
// ─────────────────────────────────────────────
NodePtr Parser::parseLinkDecl() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'enlazar'

    TypeAnnotation ret = parseType();

    if (!check(TokenType::IDENT)) {
        syncError("Se esperaba el nombre de la función externa", current().line, current().col);
    }
    std::string name = current().lexeme;
    advance();

    expect(TokenType::LPAREN, "Se esperaba '(' en la declaración enlazar");

    std::vector<TypeAnnotation> param_types;
    if (!check(TokenType::RPAREN)) {
        param_types.push_back(parseType());
        while (match(TokenType::COMMA)) {
            param_types.push_back(parseType());
        }
    }

    expect(TokenType::RPAREN, "Se esperaba ')' para cerrar la declaración enlazar");
    return std::make_unique<LinkDecl>(ret, name, std::move(param_types), ln, cl);
}

// ─────────────────────────────────────────────
//  Bloque { sentencias }
// ─────────────────────────────────────────────
NodePtr Parser::parseBlock() {
    int ln = current().line, cl = current().col;
    expect(TokenType::LBRACE, "Se esperaba '{'");
    skipNewlines();

    std::vector<NodePtr> stmts;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RBRACE)) break;

        try {
            auto stmt = parseStmt();
            if (stmt) stmts.push_back(std::move(stmt));
        } catch (const KemError&) {
            // Error ya reportado — intentar recuperarse dentro del bloque
            skipNewlines();
        }
    }

    expect(TokenType::RBRACE, "Se esperaba '}' para cerrar el bloque");
    return std::make_unique<Block>(std::move(stmts), ln, cl);
}

// ─────────────────────────────────────────────
//  Sentencias
// ─────────────────────────────────────────────
NodePtr Parser::parseStmt() {
    skipNewlines();

    // Declaración de variable: tipo nombre ...
    if (isTypeToken(current().type)) {
        return parseVarOrArrayDecl();
    }

    switch (current().type) {
        case TokenType::KW_SI:      return parseIfStmt();
        case TokenType::KW_MIENTRAS: return parseWhileStmt();
        case TokenType::KW_DEVOLVER: return parseReturnStmt();
        case TokenType::LBRACE:      return parseBlock();

        case TokenType::IDENT:
            return parseIdentStmt();

        default:
            syncError("Sentencia no reconocida: '" + current().lexeme + "'",
                      current().line, current().col);
            return nullptr;
    }
}

// ─────────────────────────────────────────────
//  Declaración de variable o arreglo
//  tipo nombre [= expr]
//  tipo nombre[N] [= [e1, ...]]
// ─────────────────────────────────────────────
NodePtr Parser::parseVarOrArrayDecl() {
    int ln = current().line, cl = current().col;
    TypeAnnotation type = parseType();

    // El nombre puede ser cualquier token no-operador (permite variables llamadas
    // "x", "y", "o", "i", etc. que colisionan con keywords cortas)
    if (isAtEnd() || check(TokenType::NEWLINE) || check(TokenType::LBRACE) ||
        check(TokenType::RBRACE) || check(TokenType::EOF_TOK)) {
        syncError("Se esperaba el nombre de la variable", current().line, current().col);
    }
    std::string name = current().lexeme;
    advance();

    // Arreglo: nombre[N]
    if (check(TokenType::LBRACKET)) {
        // El tamaño ya viene en type si se parseó tipo[N]
        // Pero la gramática KEM es: tipo nombre[N] — el [N] está después del nombre
        // Entonces parseType() ya no consumió el [N] en este caso.
        // Consumimos aquí:
        advance(); // '['
        if (!check(TokenType::INTEGER_LIT)) {
            syncError("Se esperaba el tamaño del arreglo", current().line, current().col);
        }
        type.is_array   = true;
        type.array_size = std::stoi(current().lexeme);
        advance();
        expect(TokenType::RBRACKET, "Se esperaba ']'");

        // Inicializador opcional
        std::vector<NodePtr> init;
        if (match(TokenType::EQ)) {
            expect(TokenType::LBRACKET, "Se esperaba '[' para el inicializador del arreglo");
            if (!check(TokenType::RBRACKET)) {
                init.push_back(parseExpr());
                while (match(TokenType::COMMA)) {
                    init.push_back(parseExpr());
                }
            }
            expect(TokenType::RBRACKET, "Se esperaba ']' para cerrar el inicializador");
        }

        skipNewlines();
        return std::make_unique<ArrayDecl>(type, name, std::move(init), ln, cl);
    }

    // Variable escalar
    NodePtr init;
    if (match(TokenType::EQ)) {
        init = parseExpr();
    }

    skipNewlines();
    return std::make_unique<VarDecl>(type, name, std::move(init), false, ln, cl);
}

// ─────────────────────────────────────────────
//  Sentencia que empieza con IDENT
//  Puede ser:
//    asignación:  x = expr
//    asign array: x[i] = expr
//    asign miembro: x.campo = expr
//    bucle for:   x = expr hasta expr [paso expr] { }
//    llamada:     funcion(args)
// ─────────────────────────────────────────────
NodePtr Parser::parseIdentStmt() {
    int ln = current().line, cl = current().col;
    std::string name = current().lexeme;
    advance();

    // Llamada a función: nombre(args)
    if (check(TokenType::LPAREN)) {
        advance(); // consume '('
        std::vector<NodePtr> args;
        if (!check(TokenType::RPAREN)) {
            args.push_back(parseExpr());
            while (match(TokenType::COMMA)) {
                args.push_back(parseExpr());
            }
        }
        expect(TokenType::RPAREN, "Se esperaba ')' para cerrar la llamada");
        skipNewlines();
        return std::make_unique<CallExpr>(name, std::move(args), ln, cl);
    }

    // Acceso de índice antes de asignación o for: x[i] = ...
    if (check(TokenType::LBRACKET)) {
        advance(); // '['
        auto idx = parseExpr();
        expect(TokenType::RBRACKET, "Se esperaba ']'");
        expect(TokenType::EQ, "Se esperaba '=' para la asignación");
        auto val = parseExpr();
        skipNewlines();

        auto base  = std::make_unique<IdentExpr>(name, ln, cl);
        auto index = std::make_unique<IndexExpr>(std::move(base), std::move(idx), ln, cl);
        return std::make_unique<AssignStmt>(std::move(index), std::move(val), ln, cl);
    }

    // Acceso de miembro antes de asignación: x.campo = ...
    if (check(TokenType::DOT)) {
        advance(); // '.'
        // El nombre del campo puede ser cualquier token (incluso keywords de 1 letra como "y", "o")
        if (isAtEnd() || check(TokenType::NEWLINE)) {
            syncError("Se esperaba el nombre del campo después de '.'", current().line, current().col);
        }
        std::string field = current().lexeme;
        advance();
        expect(TokenType::EQ, "Se esperaba '=' para la asignación de campo");
        auto val  = parseExpr();
        skipNewlines();

        auto base   = std::make_unique<IdentExpr>(name, ln, cl);
        auto member = std::make_unique<MemberExpr>(std::move(base), field, ln, cl);
        return std::make_unique<AssignStmt>(std::move(member), std::move(val), ln, cl);
    }

    // Asignación simple o inicio de bucle for: x = expr [hasta ...]
    if (check(TokenType::EQ)) {
        advance(); // consume '='
        auto start = parseExpr();

        // Bucle for: x = expr hasta expr [paso expr] { }
        if (check(TokenType::KW_HASTA)) {
            advance(); // consume 'hasta'
            auto end = parseExpr();

            NodePtr step;
            if (check(TokenType::KW_PASO)) {
                advance(); // consume 'paso'
                step = parseExpr();
            }

            skipNewlines();
            auto body = parseBlock();

            return std::make_unique<ForStmt>(name, std::move(start), std::move(end),
                                              std::move(step), std::move(body), ln, cl);
        }

        // Asignación simple
        skipNewlines();
        auto target = std::make_unique<IdentExpr>(name, ln, cl);
        return std::make_unique<AssignStmt>(std::move(target), std::move(start), ln, cl);
    }

    syncError("Sentencia inesperada con identificador '" + name + "'", ln, cl);
    return nullptr;
}

// ─────────────────────────────────────────────
//  si expr { } [sino { } / sino si { }]
// ─────────────────────────────────────────────
NodePtr Parser::parseIfStmt() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'si'

    auto cond = parseExpr();
    skipNewlines();
    auto then_b = parseBlock();

    NodePtr else_b;
    // skipNewlines pero solo si hay sino después
    int saved_pos = pos_;
    skipNewlines();
    if (check(TokenType::KW_SINO)) {
        advance(); // consume 'sino'
        skipNewlines();
        if (check(TokenType::KW_SI)) {
            // sino si → recursión
            else_b = parseIfStmt();
        } else {
            else_b = parseBlock();
        }
    } else {
        // No hay sino — restaurar posición para no consumir NEWLINEs de más
        pos_ = saved_pos;
    }

    return std::make_unique<IfStmt>(std::move(cond), std::move(then_b),
                                     std::move(else_b), ln, cl);
}

// ─────────────────────────────────────────────
//  mientras expr { }
// ─────────────────────────────────────────────
NodePtr Parser::parseWhileStmt() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'mientras'

    auto cond = parseExpr();
    skipNewlines();
    auto body = parseBlock();

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), ln, cl);
}

// ─────────────────────────────────────────────
//  devolver [expr]
// ─────────────────────────────────────────────
NodePtr Parser::parseReturnStmt() {
    int ln = current().line, cl = current().col;
    advance(); // consume 'devolver'

    NodePtr value;
    // Si la siguiente línea tiene contenido y NO hay newline inmediato
    if (!check(TokenType::NEWLINE) && !check(TokenType::RBRACE) && !isAtEnd()) {
        value = parseExpr();
    }

    skipNewlines();
    return std::make_unique<ReturnStmt>(std::move(value), ln, cl);
}

// ─────────────────────────────────────────────
//  Expresiones — Pratt parsing
//
//  Tabla de precedencias (mayor número = más fuerte):
//    o          → 1
//    y          → 2
//    == !=      → 3
//    < > <= >=  → 4
//    + -        → 5
//    * / %      → 6
//    no - (uni) → se manejan en parsePrimary
//    . [] ()    → se manejan en parsePostfix
// ─────────────────────────────────────────────
int Parser::binopPrec(const Token& t) const {
    switch (t.type) {
        case TokenType::KW_O:  return 1;
        case TokenType::KW_Y:  return 2;
        case TokenType::EQEQ:
        case TokenType::NEQ:   return 3;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE:   return 4;
        case TokenType::PLUS:
        case TokenType::MINUS: return 5;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT: return 6;
        default: return -1;
    }
}

bool Parser::isBinop(const Token& t) const {
    return binopPrec(t) >= 0;
}

std::string Parser::binopLexeme(const Token& t) const {
    // Para keywords lógicas usamos el lexema directamente
    return t.lexeme;
}

NodePtr Parser::parseExpr(int minPrec) {
    auto left = parsePrimary();
    left = parsePostfix(std::move(left));

    while (isBinop(current()) && binopPrec(current()) >= minPrec) {
        int ln = current().line, cl = current().col;
        std::string op = binopLexeme(current());
        int prec = binopPrec(current());
        advance(); // consume el operador

        // Asociatividad izquierda → minPrec = prec + 1
        auto right = parseExpr(prec + 1);
        right = parsePostfix(std::move(right));

        left = std::make_unique<BinaryExpr>(op, std::move(left),
                                             std::move(right), ln, cl);
    }

    return left;
}

// ─────────────────────────────────────────────
//  Primarios
// ─────────────────────────────────────────────
NodePtr Parser::parsePrimary() {
    int ln = current().line, cl = current().col;

    // Número entero
    if (check(TokenType::INTEGER_LIT)) {
        long long v = std::stoll(current().lexeme);
        advance();
        return std::make_unique<NumberLiteral>(v, ln, cl);
    }

    // Número decimal
    if (check(TokenType::FLOAT_LIT)) {
        double v = std::stod(current().lexeme);
        advance();
        return std::make_unique<FloatLiteral>(v, ln, cl);
    }

    // String
    if (check(TokenType::STRING_LIT)) {
        std::string s = current().lexeme;
        advance();
        return std::make_unique<StringLiteral>(s, ln, cl);
    }

    // verdadero / falso
    if (check(TokenType::KW_VERDADERO)) {
        advance();
        return std::make_unique<BoolLiteral>(true, ln, cl);
    }
    if (check(TokenType::KW_FALSO)) {
        advance();
        return std::make_unique<BoolLiteral>(false, ln, cl);
    }

    // Operador unario: no
    if (check(TokenType::KW_NO)) {
        advance();
        auto operand = parseExpr(7); // precedencia alta
        return std::make_unique<UnaryExpr>("no", std::move(operand), ln, cl);
    }

    // Negación unaria: -
    if (check(TokenType::MINUS)) {
        advance();
        auto operand = parseExpr(7);
        return std::make_unique<UnaryExpr>("-", std::move(operand), ln, cl);
    }

    // Agrupación: (expr)
    if (check(TokenType::LPAREN)) {
        advance(); // consume '('
        auto expr = parseExpr();
        expect(TokenType::RPAREN, "Se esperaba ')' para cerrar la expresión");
        return expr;
    }

    // Identificador o llamada a función
    if (check(TokenType::IDENT)) {
        std::string name = current().lexeme;
        advance();

        // Llamada: nombre(args)
        if (check(TokenType::LPAREN)) {
            advance(); // '('
            std::vector<NodePtr> args;
            if (!check(TokenType::RPAREN)) {
                args.push_back(parseExpr());
                while (match(TokenType::COMMA)) {
                    skipNewlines();
                    args.push_back(parseExpr());
                }
            }
            expect(TokenType::RPAREN, "Se esperaba ')' para cerrar la llamada a función");
            return std::make_unique<CallExpr>(name, std::move(args), ln, cl);
        }

        return std::make_unique<IdentExpr>(name, ln, cl);
    }

    syncError("Expresión inesperada: '" + current().lexeme + "'",
              current().line, current().col);
    return nullptr;
}

// ─────────────────────────────────────────────
//  Postfijos: . y []
//  Se aplican de izquierda a derecha sobre cualquier expr
// ─────────────────────────────────────────────
NodePtr Parser::parsePostfix(NodePtr left) {
    while (true) {
        int ln = current().line, cl = current().col;

        // Acceso a campo: expr.nombre
        // El campo puede ser cualquier token (incluso keywords cortas como "y", "o", "x")
        if (check(TokenType::DOT)) {
            advance(); // '.'
            if (isAtEnd() || check(TokenType::NEWLINE)) {
                syncError("Se esperaba el nombre del campo después de '.'",
                          current().line, current().col);
            }
            std::string field = current().lexeme;
            advance();
            left = std::make_unique<MemberExpr>(std::move(left), field, ln, cl);
            continue;
        }

        // Acceso a índice: expr[idx]
        if (check(TokenType::LBRACKET)) {
            advance(); // '['
            auto idx = parseExpr();
            expect(TokenType::RBRACKET, "Se esperaba ']' para cerrar el índice");
            left = std::make_unique<IndexExpr>(std::move(left), std::move(idx), ln, cl);
            continue;
        }

        break;
    }
    return left;
}

} // namespace kem
