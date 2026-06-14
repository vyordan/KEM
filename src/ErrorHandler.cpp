#include "kem/ErrorHandler.hpp"
#include <sstream>

namespace kem {

// ─────────────────────────────────────────────
//  phaseName
// ─────────────────────────────────────────────
const char* phaseName(Phase p) {
    switch (p) {
        case Phase::LANG_CONFIG: return "Configuración";
        case Phase::LEXER:       return "Léxico";
        case Phase::PARSER:      return "Sintáctico";
        case Phase::SEMANTIC:    return "Semántico";
        case Phase::CODEGEN:     return "Generación de código";
        case Phase::JIT:         return "JIT";
        case Phase::CLI:         return "CLI";
        default:                 return "Desconocido";
    }
}

// ─────────────────────────────────────────────
//  KemError
// ─────────────────────────────────────────────
KemError::KemError(Phase phase, const std::string& message, int line, int col)
    : phase_(phase), message_(message), line_(line), col_(col) {}

const char* KemError::what() const noexcept {
    std::ostringstream ss;
    ss << "Error [" << phaseName(phase_) << "]";

    if (line_ > 0) {
        ss << " línea " << line_;
        if (col_ > 0) {
            ss << ", col " << col_;
        }
    }

    ss << ": " << message_;
    formatted_ = ss.str();
    return formatted_.c_str();
}

// ─────────────────────────────────────────────
//  Helpers por fase
// ─────────────────────────────────────────────
void lexError(const std::string& msg, int line, int col) {
    throw KemError(Phase::LEXER, msg, line, col);
}

void parseError(const std::string& msg, int line, int col) {
    throw KemError(Phase::PARSER, msg, line, col);
}

void semanticError(const std::string& msg, int line, int col) {
    throw KemError(Phase::SEMANTIC, msg, line, col);
}

void codegenError(const std::string& msg, int line, int col) {
    throw KemError(Phase::CODEGEN, msg, line, col);
}

void configError(const std::string& msg) {
    throw KemError(Phase::LANG_CONFIG, msg, 0, 0);
}

} // namespace kem
