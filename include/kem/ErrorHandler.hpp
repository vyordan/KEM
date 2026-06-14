#pragma once

#include <stdexcept>
#include <string>

namespace kem {

// ─────────────────────────────────────────────
//  Fases del compilador — para identificar el origen del error
// ─────────────────────────────────────────────
enum class Phase {
    LANG_CONFIG,
    LEXER,
    PARSER,
    SEMANTIC,
    CODEGEN,
    JIT,
    CLI
};

const char* phaseName(Phase p);

// ─────────────────────────────────────────────
//  KemError
//  Excepción base del compilador.
//  Todas las fases lanzan este tipo — el CLI lo captura
//  y formatea el mensaje antes de imprimir a stderr.
// ─────────────────────────────────────────────
class KemError : public std::exception {
public:
    KemError(Phase phase,
             const std::string& message,
             int line = 0,
             int col  = 0);

    // Mensaje formateado para mostrar al usuario:
    // "Error [Léxico] línea 5, col 12: carácter inesperado '@'"
    const char* what() const noexcept override;

    Phase       phase()   const { return phase_;   }
    int         line()    const { return line_;     }
    int         col()     const { return col_;      }
    const std::string& message() const { return message_; }

private:
    Phase       phase_;
    std::string message_;
    int         line_;
    int         col_;
    mutable std::string formatted_;  // cache del mensaje formateado
};

// ─────────────────────────────────────────────
//  Helpers — para lanzar errores desde cada fase
//  sin repetir el boilerplate de construir KemError
// ─────────────────────────────────────────────

[[noreturn]] void lexError(const std::string& msg, int line, int col);
[[noreturn]] void parseError(const std::string& msg, int line, int col);
[[noreturn]] void semanticError(const std::string& msg, int line, int col);
[[noreturn]] void codegenError(const std::string& msg, int line = 0, int col = 0);
[[noreturn]] void configError(const std::string& msg);

} // namespace kem
