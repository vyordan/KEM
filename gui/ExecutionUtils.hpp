#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <cstdio>
#include <unistd.h>
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/SemanticAnalyzer.hpp"
#include "kem/IRGenerator.hpp"
#include "kem/JITEngine.hpp"
#include "kem/ErrorHandler.hpp"
#include "AppState.hpp"

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using US        = std::chrono::microseconds;

static int64_t us_since(TimePoint t0) {
    return std::chrono::duration_cast<US>(Clock::now() - t0).count();
}

// ── Captura con archivos temporales (sin deadlocks) ──
struct TempFileCapture {
    FILE* temp_stdout;
    FILE* temp_stderr;
    FILE* saved_stdout;
    FILE* saved_stderr;

    TempFileCapture() {
        saved_stdout = stdout;
        saved_stderr = stderr;

        temp_stdout = tmpfile();
        temp_stderr = tmpfile();
        if (!temp_stdout || !temp_stderr) {
            throw std::runtime_error("Error creando archivos temporales para captura");
        }

        if (dup2(fileno(temp_stdout), STDOUT_FILENO) == -1 ||
            dup2(fileno(temp_stderr), STDERR_FILENO) == -1) {
            throw std::runtime_error("Error redirigiendo streams");
        }
    }

    ~TempFileCapture() {
        fflush(stdout);
        fflush(stderr);
        dup2(fileno(saved_stdout), STDOUT_FILENO);
        dup2(fileno(saved_stderr), STDERR_FILENO);
        fclose(temp_stdout);
        fclose(temp_stderr);
    }

    std::string readStdout() {
        rewind(temp_stdout);
        std::string result;
        char buf[4096];
        while (fgets(buf, sizeof(buf), temp_stdout))
            result += buf;
        return result;
    }

    std::string readStderr() {
        rewind(temp_stderr);
        std::string result;
        char buf[4096];
        while (fgets(buf, sizeof(buf), temp_stderr))
            result += buf;
        return result;
    }
};

// ── AST Printer ──
static std::string printAST(kem::Program* prog) {
    struct ASTPrinter : kem::Visitor {
        int indent = 0;
        std::ostringstream out;
        std::string pad() { return std::string(indent * 2, ' '); }

        void visit(kem::Program& n) override {
            out << pad() << "Program\n";
            indent++;
            for (auto& d : n.decls) d->accept(*this);
            if (n.main_block) {
                out << pad() << "inicio:\n";
                indent++; n.main_block->accept(*this); indent--;
            }
            indent--;
        }
        void visit(kem::FuncDef& n) override {
            out << pad() << "FuncDef '" << n.name
                << "' → " << n.return_type.toString() << "\n";
            indent++; n.body->accept(*this); indent--;
        }
        void visit(kem::ProcDef& n) override {
            out << pad() << "ProcDef '" << n.name << "'\n";
            indent++; n.body->accept(*this); indent--;
        }
        void visit(kem::StructDef& n) override {
            out << pad() << "StructDef '" << n.name << "' ["
                << n.fields.size() << " campos]\n";
        }
        void visit(kem::LinkDecl& n) override {
            out << pad() << "LinkDecl '" << n.name << "'\n";
        }
        void visit(kem::Block& n) override {
            out << pad() << "Block [" << n.stmts.size() << " stmts]\n";
            indent++;
            for (auto& s : n.stmts) s->accept(*this);
            indent--;
        }
        void visit(kem::VarDecl& n) override {
            out << pad() << "VarDecl " << n.type.toString()
                << " '" << n.name << "'"
                << (n.init ? " = ..." : "") << "\n";
            if (n.init) { indent++; n.init->accept(*this); indent--; }
        }
        void visit(kem::ArrayDecl& n) override {
            out << pad() << "ArrayDecl " << n.type.toString()
                << " '" << n.name << "' ["
                << n.init.size() << " inits]\n";
        }
        void visit(kem::AssignStmt& n) override {
            out << pad() << "Assign\n";
            indent++;
            out << pad() << "target:\n";
            indent++; n.target->accept(*this); indent--;
            out << pad() << "value:\n";
            indent++; n.value->accept(*this); indent--;
            indent--;
        }
        void visit(kem::IfStmt& n) override {
            out << pad() << "IfStmt\n";
            indent++;
            out << pad() << "cond:\n";
            indent++; n.condition->accept(*this); indent--;
            out << pad() << "then:\n";
            indent++; n.then_block->accept(*this); indent--;
            if (n.else_block) {
                out << pad() << "else:\n";
                indent++; n.else_block->accept(*this); indent--;
            }
            indent--;
        }
        void visit(kem::WhileStmt& n) override {
            out << pad() << "WhileStmt\n";
            indent++;
            out << pad() << "cond:\n";
            indent++; n.condition->accept(*this); indent--;
            out << pad() << "body:\n";
            indent++; n.body->accept(*this); indent--;
            indent--;
        }
        void visit(kem::ForStmt& n) override {
            out << pad() << "ForStmt '" << n.iter_var << "'\n";
            indent++;
            out << pad() << "start:\n"; indent++; n.start->accept(*this); indent--;
            out << pad() << "end:\n";   indent++; n.end->accept(*this);   indent--;
            if (n.step) {
                out << pad() << "step:\n"; indent++; n.step->accept(*this); indent--;
            }
            out << pad() << "body:\n";  indent++; n.body->accept(*this);   indent--;
            indent--;
        }
        void visit(kem::ReturnStmt& n) override {
            out << pad() << "Return" << (n.value ? ":\n" : " (void)\n");
            if (n.value) { indent++; n.value->accept(*this); indent--; }
        }
        void visit(kem::NumberLiteral& n)  override { out << pad() << "Number " << n.value << "\n"; }
        void visit(kem::FloatLiteral& n)   override { out << pad() << "Float " << n.value << "\n"; }
        void visit(kem::StringLiteral& n)  override { out << pad() << "String \"" << n.value << "\"\n"; }
        void visit(kem::BoolLiteral& n)    override { out << pad() << "Bool " << (n.value?"verdadero":"falso") << "\n"; }
        void visit(kem::IdentExpr& n)      override { out << pad() << "Ident '" << n.name << "'\n"; }
        void visit(kem::BinaryExpr& n) override {
            out << pad() << "Binary '" << n.op << "'\n";
            indent++;
            n.left->accept(*this);
            n.right->accept(*this);
            indent--;
        }
        void visit(kem::UnaryExpr& n) override {
            out << pad() << "Unary '" << n.op << "'\n";
            indent++; n.operand->accept(*this); indent--;
        }
        void visit(kem::CallExpr& n) override {
            out << pad() << "Call '" << n.callee
                << "' [" << n.args.size() << " args]\n";
            indent++;
            for (auto& a : n.args) a->accept(*this);
            indent--;
        }
        void visit(kem::IndexExpr& n) override {
            out << pad() << "Index\n";
            indent++;
            out << pad() << "obj:\n";   indent++; n.object->accept(*this); indent--;
            out << pad() << "index:\n"; indent++; n.index->accept(*this);  indent--;
            indent--;
        }
        void visit(kem::MemberExpr& n) override {
            out << pad() << "Member '." << n.field << "'\n";
            indent++; n.object->accept(*this); indent--;
        }
    };

    ASTPrinter printer;
    prog->accept(printer);
    return printer.out.str();
}

// ── Ejecución principal (captura opcional) ──
inline ExecutionResult executeKemCode(
    const std::string&      source,
    const kem::LangConfig&  langConfig,
    bool emitTokens,
    bool emitAST,
    bool emitIR,
    bool benchmark,
    bool captureOutput
) {
    ExecutionResult res;
    res.success = false;

    TempFileCapture* capture = nullptr;
    try {
        if (captureOutput) {
            capture = new TempFileCapture();
        }

        // Lexer
        auto t0 = Clock::now();
        kem::Lexer lexer(source, langConfig);
        auto tokens = lexer.tokenize();
        int64_t t_lex = us_since(t0);

        if (emitTokens) {
            std::ostringstream tokenTable;
            tokenTable << "\n┌──────────┬──────────────────────┬──────────────────────────┐\n";
            tokenTable <<   "│  Línea   │       Tipo           │        Lexema            │\n";
            tokenTable <<   "├──────────┼──────────────────────┼──────────────────────────┤\n";
            for (const auto& tok : tokens) {
                std::string pos    = std::to_string(tok.line) + ":" + std::to_string(tok.col);
                std::string type   = kem::tokenTypeName(tok.type);
                std::string lexeme = tok.lexeme == "\n" ? "↵" : tok.lexeme;
                auto pad = [](std::string s, int w) {
                    while ((int)s.size() < w) s += ' ';
                    if ((int)s.size() > w) s = s.substr(0, w-1) + "…";
                    return s;
                };
                tokenTable << "│ " << pad(pos,8) << " │ " << pad(type,20)
                           << " │ " << pad(lexeme,24) << " │\n";
            }
            tokenTable << "└──────────┴──────────────────────┴──────────────────────────┘\n";
            tokenTable << "  Total: " << tokens.size() << " tokens\n\n";
            res.tokens = tokenTable.str();
        }

        // Parser
        t0 = Clock::now();
        kem::Parser parser(std::move(tokens));
        auto prog = parser.parse();
        int64_t t_parse = us_since(t0);

        if (emitAST) {
            res.ast = printAST(prog.get());
        }

        // Semántico
        t0 = Clock::now();
        kem::SemanticAnalyzer sem;
        sem.analyze(*prog);
        int64_t t_sem = us_since(t0);

        // Codegen
        t0 = Clock::now();
        kem::IRGenerator gen;
        gen.generate(*prog);
        int64_t t_codegen = us_since(t0);

        if (emitIR) {
            std::ostringstream irStream;
            auto oldBuf = std::cout.rdbuf(irStream.rdbuf());
            gen.emitIR();
            std::cout.rdbuf(oldBuf);
            res.ir = irStream.str();
        }

        // JIT + Ejecución
        t0 = Clock::now();
        kem::JITEngine jit;
        jit.addModule(gen.takeModule(), gen.context());
        int64_t t_jit = us_since(t0);

        t0 = Clock::now();
        int64_t result = jit.run();
        int64_t t_exec = us_since(t0);

        // ¡Importante! Vaciar los streams para que la salida llegue a la terminal
        fflush(stdout);
        fflush(stderr);

        if (benchmark) {
            int64_t t_total = t_lex + t_parse + t_sem + t_codegen + t_jit + t_exec;
            auto bar = [](int64_t t, int64_t total, int width = 20) -> std::string {
                int filled = total > 0 ? (int)((double)t / total * width) : 0;
                return std::string(filled, '#') + std::string(width - filled, '.');
            };
            std::ostringstream bench;
            bench << "\n── Benchmark KEM ──────────────────────────────\n";
            bench << "  Fase              │   Tiempo (µs)  │ Proporción\n";
            bench << "──────────────────────────────────────────────\n";
            auto row = [&](const char* name, int64_t t) {
                bench << "  " << name;
                int pad = 16 - (int)strlen(name);
                bench << std::string(pad, ' ') << "│ ";
                bench << t << " µs" << std::string(14 - std::to_string(t).size() - 2, ' ') << "│ ";
                bench << bar(t, t_total) << "\n";
            };
            row("Lexer",        t_lex);
            row("Parser",       t_parse);
            row("Semántico",    t_sem);
            row("Codegen IR",   t_codegen);
            row("JIT compile",  t_jit);
            row("Ejecución",    t_exec);
            bench << "──────────────────────────────────────────────\n";
            bench << "  Total             │ " << t_total << " µs\n\n";
            res.benchmark = bench.str();
        }

        if (capture) {
            res.output = capture->readStdout();
            res.errors += capture->readStderr();
            delete capture;
            capture = nullptr;
        } else {
            res.output = "(salida mostrada en terminal)";
        }

        res.exitCode = 0;
        res.success = true;
    }
    catch (const kem::KemError& e) {
        res.errors = e.what();
        res.exitCode = 1;
    }
    catch (const std::exception& e) {
        res.errors = std::string("Error interno: ") + e.what();
        res.exitCode = 1;
    }

    if (capture) {
        delete capture;
    }
    return res;
}