#include "kem/Lexer.hpp"
#include "kem/ErrorHandler.hpp"
#include "kem/ErrorMessages.hpp"

#include <sstream>

namespace kem {

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Lexer::Lexer(std::string src, const LangConfig& config)
    : src_(std::move(src)), config_(config) {}

// ─────────────────────────────────────────────
//  Navegación
// ─────────────────────────────────────────────
char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return src_[pos_];
}

char Lexer::peek(int offset) const {
    size_t idx = static_cast<size_t>(pos_ + offset);
    if (idx >= src_.size()) return '\0';
    return src_[idx];
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= static_cast<int>(src_.size());
}

// ─────────────────────────────────────────────
//  Marca el inicio del token actual
// ─────────────────────────────────────────────
void Lexer::markTokenStart() {
    tokenStartLine_ = line_;
    tokenStartCol_  = col_;
}

// ─────────────────────────────────────────────
//  Builders de tokens
// ─────────────────────────────────────────────
Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return Token(type, lexeme, tokenStartLine_, tokenStartCol_);
}

Token Lexer::makeToken(TokenType type, char c) const {
    return Token(type, std::string(1, c), tokenStartLine_, tokenStartCol_);
}

// ─────────────────────────────────────────────
//  Utilidades de clasificación de caracteres
// ─────────────────────────────────────────────
bool Lexer::isDigit(char c)    const { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c)    const { return (c >= 'a' && c <= 'z') ||
                                              (c >= 'A' && c <= 'Z') ||
                                               c == '_'; }
bool Lexer::isAlphaNum(char c) const { return isAlpha(c) || isDigit(c); }

// ─────────────────────────────────────────────
//  Saltar espacios y tabs (NO newlines)
//  Los newlines son tokens importantes para el Parser.
// ─────────────────────────────────────────────
void Lexer::skipWhitespaceNoNewline() {
    while (!isAtEnd() && (current() == ' ' || current() == '\t' || current() == '\r')) {
        advance();
    }
}

// ─────────────────────────────────────────────
//  Comentarios — no producen tokens
// ─────────────────────────────────────────────
void Lexer::skipLineComment() {
    // Consume hasta fin de línea (pero NO el '\n' — ese es un NEWLINE token)
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    // Consume hasta '*/'
    while (!isAtEnd()) {
        if (current() == '*' && peek() == '/') {
            advance(); // '*'
            advance(); // '/'
            return;
        }
        advance();
    }
    // Si llegamos aquí, el bloque nunca se cerró
    lexError(errorMessages().format("LEX_COMENTARIO_BLOQUE_SIN_CERRAR"), line_, col_);
}

void Lexer::skipKemLineComment() {
    // 'comentario' ya fue consumido por readIdentOrKw.
    // Consumir el resto de la línea.
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

void Lexer::skipKemBlockComment() {
    // 'comentario{' — consumir hasta '}'
    // El '{' ya fue detectado en readIdentOrKw.
    advance(); // consume '{'
    while (!isAtEnd()) {
        if (current() == '}') {
            advance();
            return;
        }
        advance();
    }
    lexError(errorMessages().format("LEX_COMENTARIO_KEM_SIN_CERRAR"), line_, col_);
}

// ─────────────────────────────────────────────
//  readNumber — INTEGER_LIT o FLOAT_LIT
// ─────────────────────────────────────────────
Token Lexer::readNumber() {
    markTokenStart();
    std::string num;
    bool is_float = false;

    while (!isAtEnd() && isDigit(current())) {
        num += advance();
    }

    // Parte decimal: '.' seguido de dígito
    if (!isAtEnd() && current() == '.' && isDigit(peek())) {
        is_float = true;
        num += advance(); // '.'
        while (!isAtEnd() && isDigit(current())) {
            num += advance();
        }
    }

    return makeToken(is_float ? TokenType::FLOAT_LIT : TokenType::INTEGER_LIT, num);
}

// ─────────────────────────────────────────────
//  readString — STRING_LIT
// ─────────────────────────────────────────────
Token Lexer::readString() {
    markTokenStart();
    advance(); // consume la comilla de apertura '"'

    std::string value;
    while (!isAtEnd() && current() != '"') {
        if (current() == '\n') {
            lexError(errorMessages().format("LEX_STRING_NEWLINE"),
                     tokenStartLine_, tokenStartCol_);
        }
        // Secuencias de escape básicas
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n':  value += '\n'; advance(); break;
                case 't':  value += '\t'; advance(); break;
                case '"':  value += '"';  advance(); break;
                case '\\': value += '\\'; advance(); break;
                default:
                    lexError(errorMessages().format("LEX_ESCAPE_DESCONOCIDO",
                             {std::string(1, current())}), line_, col_);
            }
        } else {
            value += advance();
        }
    }

    if (isAtEnd()) {
        lexError(errorMessages().format("LEX_STRING_SIN_CERRAR"),
                 tokenStartLine_, tokenStartCol_);
    }

    advance(); // consume la comilla de cierre '"'
    return makeToken(TokenType::STRING_LIT, value);
}

// ─────────────────────────────────────────────
//  readIdentOrKw — IDENT o keyword del idioma
// ─────────────────────────────────────────────
Token Lexer::readIdentOrKw() {
    markTokenStart();
    std::string word;

    while (!isAtEnd() && isAlphaNum(current())) {
        word += advance();
    }

    // ── Comentarios nativos de KEM ("comentario" / "comentario{") ──────
    // Las palabras de comentario ya NO están hardcodeadas — se leen desde
    // el archivo de idioma vía LangConfig (claves "_comment_line" y
    // "_comment_block"). Esto permite que cada idioma defina su propia
    // palabra: "comentario" en español, "comment" en inglés, etc.
    // Si el idioma no define estas claves, el estilo nativo queda
    // deshabilitado para ese idioma — // y /* */ siguen funcionando
    // siempre porque no dependen de ninguna palabra.

    const std::string& block_word = config_.commentBlockWord();
    const std::string& line_word  = config_.commentLineWord();

    // Detectar comentario de bloque: "<palabra>{"
    // Se verifica primero porque comparte el mismo prefijo que el de línea.
    if (!block_word.empty() && word == block_word &&
        !isAtEnd() && current() == '{') {
        skipKemBlockComment();
        return Token(TokenType::UNKNOWN, "", tokenStartLine_, tokenStartCol_);
        // UNKNOWN aquí significa "descarta este token" — el tokenizador lo filtrará
    }

    // Detectar comentario de línea: "<palabra> <resto>"
    // Solo si la palabra completa coincide exactamente — un identificador
    // como "comentarioX" en español (o "commentX" en inglés) sigue siendo
    // un identificador válido, no dispara el comentario.
    if (!line_word.empty() && word == line_word) {
        skipKemLineComment();
        return Token(TokenType::UNKNOWN, "", tokenStartLine_, tokenStartCol_);
    }

    // Resolver keyword vs identificador via LangConfig
    TokenType type = config_.resolve(word);
    return makeToken(type, word);
}

// ─────────────────────────────────────────────
//  readSymbol — operadores y delimitadores
// ─────────────────────────────────────────────
Token Lexer::readSymbol() {
    markTokenStart();
    char c = advance();

    switch (c) {
        case '+': return makeToken(TokenType::PLUS,     c);
        case '-': return makeToken(TokenType::MINUS,    c);
        case '*': return makeToken(TokenType::STAR,     c);
        case '%': return makeToken(TokenType::PERCENT,  c);
        case '(': return makeToken(TokenType::LPAREN,   c);
        case ')': return makeToken(TokenType::RPAREN,   c);
        case '{': return makeToken(TokenType::LBRACE,   c);
        case '}': return makeToken(TokenType::RBRACE,   c);
        case '[': return makeToken(TokenType::LBRACKET, c);
        case ']': return makeToken(TokenType::RBRACKET, c);
        case ',': return makeToken(TokenType::COMMA,    c);
        case '.': return makeToken(TokenType::DOT,      c);

        case '/':
            // Comentario de línea: //
            if (!isAtEnd() && current() == '/') {
                advance(); // segundo '/'
                skipLineComment();
                return Token(TokenType::UNKNOWN, "", tokenStartLine_, tokenStartCol_);
            }
            // Comentario de bloque: /* */
            if (!isAtEnd() && current() == '*') {
                advance(); // '*'
                skipBlockComment();
                return Token(TokenType::UNKNOWN, "", tokenStartLine_, tokenStartCol_);
            }
            return makeToken(TokenType::SLASH, c);

        case '=':
            if (!isAtEnd() && current() == '=') {
                advance();
                return makeToken(TokenType::EQEQ, "==");
            }
            return makeToken(TokenType::EQ, c);

        case '!':
            if (!isAtEnd() && current() == '=') {
                advance();
                return makeToken(TokenType::NEQ, "!=");
            }
            lexError(errorMessages().format("LEX_NOT_SIN_IGUAL"), line_, col_);

        case '<':
            if (!isAtEnd() && current() == '=') {
                advance();
                return makeToken(TokenType::LTE, "<=");
            }
            return makeToken(TokenType::LT, c);

        case '>':
            if (!isAtEnd() && current() == '=') {
                advance();
                return makeToken(TokenType::GTE, ">=");
            }
            return makeToken(TokenType::GT, c);

        default: {
            lexError(errorMessages().format("LEX_CHAR_INESPERADO",
                     {std::string(1, c)}), tokenStartLine_, tokenStartCol_);
        }
    }

    // Nunca llegamos aquí, pero el compilador lo requiere
    return makeToken(TokenType::UNKNOWN, c);
}

// ─────────────────────────────────────────────
//  readNewline
// ─────────────────────────────────────────────
Token Lexer::readNewline() {
    markTokenStart();
    // Consumir uno o más newlines consecutivos — para el Parser
    // importa saber que "hay al menos un salto de línea", no cuántos.
    while (!isAtEnd() && (current() == '\n' || current() == '\r')) {
        advance();
    }
    return makeToken(TokenType::NEWLINE, "\\n");
}

// ─────────────────────────────────────────────
//  tokenize — punto de entrada principal
// ─────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(256); // reserva inicial razonable

    while (true) {
        skipWhitespaceNoNewline();

        if (isAtEnd()) {
            tokens.emplace_back(TokenType::EOF_TOK, "", line_, col_);
            break;
        }

        char c = current();

        // Newline
        if (c == '\n' || c == '\r') {
            Token t = readNewline();
            // No emitir NEWLINE si el último token ya fue NEWLINE
            // (evita múltiples NEWLINEs consecutivos al Parser)
            if (!tokens.empty() && tokens.back().type != TokenType::NEWLINE) {
                tokens.push_back(t);
            }
            continue;
        }

        // Número
        if (isDigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }

        // String
        if (c == '"') {
            tokens.push_back(readString());
            continue;
        }

        // Identificador o keyword
        if (isAlpha(c)) {
            Token t = readIdentOrKw();
            // Descartar tokens UNKNOWN (comentarios)
            if (t.type != TokenType::UNKNOWN) {
                tokens.push_back(t);
            }
            continue;
        }

        // Símbolo / operador
        Token t = readSymbol();
        if (t.type != TokenType::UNKNOWN) {
            tokens.push_back(t);
        }
    }

    return tokens;
}

} // namespace kem
