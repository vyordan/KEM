#include "kem/Token.hpp"

namespace kem {

const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::INTEGER_LIT:  return "INTEGER_LIT";
        case TokenType::FLOAT_LIT:    return "FLOAT_LIT";
        case TokenType::STRING_LIT:   return "STRING_LIT";
        case TokenType::IDENT:        return "IDENT";
        case TokenType::KW_ENTERO:    return "KW_ENTERO";
        case TokenType::KW_DECIMAL:   return "KW_DECIMAL";
        case TokenType::KW_TEXTO:     return "KW_TEXTO";
        case TokenType::KW_BOOLEANO:  return "KW_BOOLEANO";
        case TokenType::KW_FUNCION:   return "KW_FUNCION";
        case TokenType::KW_PROC:      return "KW_PROC";
        case TokenType::KW_DEVOLVER:  return "KW_DEVOLVER";
        case TokenType::KW_INICIO:    return "KW_INICIO";
        case TokenType::KW_SI:        return "KW_SI";
        case TokenType::KW_SINO:      return "KW_SINO";
        case TokenType::KW_MIENTRAS:  return "KW_MIENTRAS";
        case TokenType::KW_HASTA:     return "KW_HASTA";
        case TokenType::KW_PASO:      return "KW_PASO";
        case TokenType::KW_VERDADERO: return "KW_VERDADERO";
        case TokenType::KW_FALSO:     return "KW_FALSO";
        case TokenType::KW_Y:         return "KW_Y";
        case TokenType::KW_O:         return "KW_O";
        case TokenType::KW_NO:        return "KW_NO";
        case TokenType::KW_ESTRUCTURA:return "KW_ESTRUCTURA";
        case TokenType::KW_REF:       return "KW_REF";
        case TokenType::KW_ENLAZAR:   return "KW_ENLAZAR";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::STAR:         return "STAR";
        case TokenType::SLASH:        return "SLASH";
        case TokenType::PERCENT:      return "PERCENT";
        case TokenType::EQ:           return "EQ";
        case TokenType::EQEQ:         return "EQEQ";
        case TokenType::NEQ:          return "NEQ";
        case TokenType::LT:           return "LT";
        case TokenType::GT:           return "GT";
        case TokenType::LTE:          return "LTE";
        case TokenType::GTE:          return "GTE";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::LBRACE:       return "LBRACE";
        case TokenType::RBRACE:       return "RBRACE";
        case TokenType::LBRACKET:     return "LBRACKET";
        case TokenType::RBRACKET:     return "RBRACKET";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::DOT:          return "DOT";
        case TokenType::NEWLINE:      return "NEWLINE";
        case TokenType::EOF_TOK:      return "EOF";
        case TokenType::UNKNOWN:      return "UNKNOWN";
        default:                      return "?";
    }
}

} // namespace kem
