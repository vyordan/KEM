#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/SemanticAnalyzer.hpp"
#include "kem/IRGenerator.hpp"
#include "kem/JITEngine.hpp"
#include "kem/ErrorHandler.hpp"
#include "kem/Token.hpp"
#include "kem/AST.hpp"

// ─────────────────────────────────────────────
//  Tipos de tiempo
// ─────────────────────────────────────────────
using Clock    = std::chrono::high_resolution_clock;
using TimePoint= std::chrono::time_point<Clock>;
using US       = std::chrono::microseconds;

static int64_t us_since(TimePoint t0) {
    return std::chrono::duration_cast<US>(Clock::now() - t0).count();
}

// ─────────────────────────────────────────────
//  Opciones CLI
// ─────────────────────────────────────────────
struct CliOptions {
    std::string source_file;
    std::string lang_file  = "langs/espanol.json";
    bool emit_tokens       = false;
    bool emit_ir           = false;
    bool emit_ast          = false;
    bool benchmark         = false;
    bool help              = false;
};

void printUsage(const char* prog) {
    std::cerr
        << "Uso: " << prog << " [opciones] archivo.kem\n\n"
        << "Opciones:\n"
        << "  --lang=<archivo.json>   Idioma (por defecto: langs/espanol.json)\n"
        << "  --emit-tokens           Imprime los tokens y termina\n"
        << "  --emit-ir               Imprime el LLVM IR y termina\n"
        << "  --emit-ast              Imprime el AST y termina\n"
        << "  --benchmark             Muestra tiempos de cada fase en µs\n"
        << "  --help                  Muestra esta ayuda\n\n"
        << "Ejemplos:\n"
        << "  " << prog << " programa.kem\n"
        << "  " << prog << " --emit-ir programa.kem\n"
        << "  " << prog << " --benchmark programa.kem\n"
        << "  " << prog << " --lang=langs/english.json programa.kem\n";
}

CliOptions parseArgs(int argc, char* argv[]) {
    CliOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--help" || arg == "-h") opts.help        = true;
        else if (arg == "--emit-tokens")         opts.emit_tokens  = true;
        else if (arg == "--emit-ir")             opts.emit_ir      = true;
        else if (arg == "--emit-ast")            opts.emit_ast     = true;
        else if (arg == "--benchmark")           opts.benchmark    = true;
        else if (arg.substr(0,7) == "--lang=")  opts.lang_file    = arg.substr(7);
        else if (!arg.empty() && arg[0] != '-') opts.source_file  = arg;
        else {
            std::cerr << "Opción desconocida: '" << arg << "'\n";
            std::exit(1);
        }
    }
    return opts;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw kem::KemError(kem::Phase::CLI,
            "No se puede abrir el archivo: '" + path + "'");
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ─────────────────────────────────────────────
//  Tabla de tokens
// ─────────────────────────────────────────────
void printTokens(const std::vector<kem::Token>& tokens) {
    std::cout << "\n┌──────────┬──────────────────────┬──────────────────────────┐\n";
    std::cout <<   "│  Línea   │       Tipo           │        Lexema            │\n";
    std::cout <<   "├──────────┼──────────────────────┼──────────────────────────┤\n";
    for (const auto& tok : tokens) {
        std::string pos    = std::to_string(tok.line) + ":" + std::to_string(tok.col);
        std::string type   = kem::tokenTypeName(tok.type);
        std::string lexeme = tok.lexeme == "\n" ? "↵" : tok.lexeme;
        auto pad = [](std::string s, int w) {
            while ((int)s.size() < w) s += ' ';
            if ((int)s.size() > w) s = s.substr(0, w-1) + "…";
            return s;
        };
        std::cout << "│ " << pad(pos,8) << " │ " << pad(type,20)
                  << " │ " << pad(lexeme,24) << " │\n";
    }
    std::cout << "└──────────┴──────────────────────┴──────────────────────────┘\n";
    std::cout << "  Total: " << tokens.size() << " tokens\n\n";
}

// ─────────────────────────────────────────────
//  AST printer — Visitor que imprime el árbol
// ─────────────────────────────────────────────
struct ASTPrinter : kem::Visitor {
    int indent = 0;
    std::string pad() { return std::string(indent * 2, ' '); }

    void visit(kem::Program& n) override {
        std::cout << pad() << "Program\n";
        indent++;
        for (auto& d : n.decls) d->accept(*this);
        if (n.main_block) {
            std::cout << pad() << "inicio:\n";
            indent++; n.main_block->accept(*this); indent--;
        }
        indent--;
    }
    void visit(kem::FuncDef& n) override {
        std::cout << pad() << "FuncDef '" << n.name
                  << "' → " << n.return_type.toString() << "\n";
        indent++; n.body->accept(*this); indent--;
    }
    void visit(kem::ProcDef& n) override {
        std::cout << pad() << "ProcDef '" << n.name << "'\n";
        indent++; n.body->accept(*this); indent--;
    }
    void visit(kem::StructDef& n) override {
        std::cout << pad() << "StructDef '" << n.name << "' ["
                  << n.fields.size() << " campos]\n";
    }
    void visit(kem::LinkDecl& n) override {
        std::cout << pad() << "LinkDecl '" << n.name << "'\n";
    }
    void visit(kem::Block& n) override {
        std::cout << pad() << "Block [" << n.stmts.size() << " stmts]\n";
        indent++;
        for (auto& s : n.stmts) s->accept(*this);
        indent--;
    }
    void visit(kem::VarDecl& n) override {
        std::cout << pad() << "VarDecl " << n.type.toString()
                  << " '" << n.name << "'"
                  << (n.init ? " = ..." : "") << "\n";
        if (n.init) { indent++; n.init->accept(*this); indent--; }
    }
    void visit(kem::ArrayDecl& n) override {
        std::cout << pad() << "ArrayDecl " << n.type.toString()
                  << " '" << n.name << "' ["
                  << n.init.size() << " inits]\n";
    }
    void visit(kem::AssignStmt& n) override {
        std::cout << pad() << "Assign\n";
        indent++;
        std::cout << pad() << "target:\n";
        indent++; n.target->accept(*this); indent--;
        std::cout << pad() << "value:\n";
        indent++; n.value->accept(*this); indent--;
        indent--;
    }
    void visit(kem::IfStmt& n) override {
        std::cout << pad() << "IfStmt\n";
        indent++;
        std::cout << pad() << "cond:\n";
        indent++; n.condition->accept(*this); indent--;
        std::cout << pad() << "then:\n";
        indent++; n.then_block->accept(*this); indent--;
        if (n.else_block) {
            std::cout << pad() << "else:\n";
            indent++; n.else_block->accept(*this); indent--;
        }
        indent--;
    }
    void visit(kem::WhileStmt& n) override {
        std::cout << pad() << "WhileStmt\n";
        indent++;
        std::cout << pad() << "cond:\n";
        indent++; n.condition->accept(*this); indent--;
        std::cout << pad() << "body:\n";
        indent++; n.body->accept(*this); indent--;
        indent--;
    }
    void visit(kem::ForStmt& n) override {
        std::cout << pad() << "ForStmt '" << n.iter_var << "'\n";
        indent++;
        std::cout << pad() << "start:\n"; indent++; n.start->accept(*this); indent--;
        std::cout << pad() << "end:\n";   indent++; n.end->accept(*this);   indent--;
        if (n.step) {
            std::cout << pad() << "step:\n"; indent++; n.step->accept(*this); indent--;
        }
        std::cout << pad() << "body:\n";  indent++; n.body->accept(*this);   indent--;
        indent--;
    }
    void visit(kem::ReturnStmt& n) override {
        std::cout << pad() << "Return" << (n.value ? ":\n" : " (void)\n");
        if (n.value) { indent++; n.value->accept(*this); indent--; }
    }
    void visit(kem::NumberLiteral& n)  override { std::cout << pad() << "Number " << n.value << "\n"; }
    void visit(kem::FloatLiteral& n)   override { std::cout << pad() << "Float " << n.value << "\n"; }
    void visit(kem::StringLiteral& n)  override { std::cout << pad() << "String \"" << n.value << "\"\n"; }
    void visit(kem::BoolLiteral& n)    override { std::cout << pad() << "Bool " << (n.value?"verdadero":"falso") << "\n"; }
    void visit(kem::IdentExpr& n)      override { std::cout << pad() << "Ident '" << n.name << "'\n"; }
    void visit(kem::BinaryExpr& n) override {
        std::cout << pad() << "Binary '" << n.op << "'\n";
        indent++;
        n.left->accept(*this);
        n.right->accept(*this);
        indent--;
    }
    void visit(kem::UnaryExpr& n) override {
        std::cout << pad() << "Unary '" << n.op << "'\n";
        indent++; n.operand->accept(*this); indent--;
    }
    void visit(kem::CallExpr& n) override {
        std::cout << pad() << "Call '" << n.callee
                  << "' [" << n.args.size() << " args]\n";
        indent++;
        for (auto& a : n.args) a->accept(*this);
        indent--;
    }
    void visit(kem::IndexExpr& n) override {
        std::cout << pad() << "Index\n";
        indent++;
        std::cout << pad() << "obj:\n";   indent++; n.object->accept(*this); indent--;
        std::cout << pad() << "index:\n"; indent++; n.index->accept(*this);  indent--;
        indent--;
    }
    void visit(kem::MemberExpr& n) override {
        std::cout << pad() << "Member '." << n.field << "'\n";
        indent++; n.object->accept(*this); indent--;
    }
};

// ─────────────────────────────────────────────
//  Tabla de benchmark
// ─────────────────────────────────────────────
void printBenchmark(int64_t t_lang, int64_t t_lex, int64_t t_parse,
                    int64_t t_sem, int64_t t_codegen,
                    int64_t t_jit, int64_t t_exec) {
    int64_t t_total = t_lang + t_lex + t_parse + t_sem +
                      t_codegen + t_jit + t_exec;

    auto bar = [](int64_t t, int64_t total, int width=20) -> std::string {
        int filled = total > 0 ? (int)((double)t / total * width) : 0;
        return std::string(filled, '#') + std::string(width - filled, '.');
    };

    std::cout << "\n── Benchmark KEM ──────────────────────────────\n";
    std::cout << "  Fase              │   Tiempo (µs)  │ Proporción\n";
    std::cout << "──────────────────────────────────────────────\n";

    auto row = [&](const char* name, int64_t t) {
        std::cout << "  " << name;
        int pad = 16 - (int)strlen(name);
        std::cout << std::string(pad, ' ') << "│ ";
        std::string ts = std::to_string(t) + " µs";
        std::cout << ts << std::string(14 - (int)ts.size(), ' ') << "│ ";
        std::cout << bar(t, t_total) << "\n";
    };

    row("LangConfig",   t_lang);
    row("Lexer",        t_lex);
    row("Parser",       t_parse);
    row("Semántico",    t_sem);
    row("Codegen IR",   t_codegen);
    row("JIT compile",  t_jit);
    row("Ejecución",    t_exec);
    std::cout << "──────────────────────────────────────────────\n";
    std::cout << "  Total             │ " << t_total << " µs\n\n";
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    CliOptions opts = parseArgs(argc, argv);

    if (opts.help || opts.source_file.empty()) {
        printUsage(argv[0]);
        return opts.help ? 0 : 1;
    }

    try {
        // ── Fase 0: Idioma ─────────────────────────
        auto t0 = Clock::now();
        kem::LangConfig config(opts.lang_file);
        int64_t t_lang = us_since(t0);

        if (!opts.benchmark)
            std::cout << "Idioma: " << config.langName() << "\n";

        // ── Fase 1: Leer fuente ────────────────────
        std::string source = readFile(opts.source_file);

        // ── Fase 2: Lexer ──────────────────────────
        t0 = Clock::now();
        kem::Lexer lexer(source, config);
        auto tokens = lexer.tokenize();
        int64_t t_lex = us_since(t0);

        if (opts.emit_tokens) { printTokens(tokens); return 0; }

        // ── Fase 3: Parser ─────────────────────────
        t0 = Clock::now();
        kem::Parser parser(std::move(tokens));
        auto prog = parser.parse();
        int64_t t_parse = us_since(t0);

        if (opts.emit_ast) {
            ASTPrinter printer;
            prog->accept(printer);
            return 0;
        }

        // ── Fase 4: Semántico ──────────────────────
        t0 = Clock::now();
        kem::SemanticAnalyzer sem;
        sem.analyze(*prog);
        int64_t t_sem = us_since(t0);

        // ── Fase 5: Codegen ────────────────────────
        t0 = Clock::now();
        kem::IRGenerator gen;
        gen.generate(*prog);
        int64_t t_codegen = us_since(t0);

        if (opts.emit_ir) { gen.emitIR(); return 0; }

        // ── Fase 6: JIT compile ────────────────────
        t0 = Clock::now();
        kem::JITEngine jit;
        jit.addModule(gen.takeModule(), gen.context());
        int64_t t_jit = us_since(t0);

        // ── Fase 7: Ejecución ──────────────────────
        t0 = Clock::now();
        int64_t result = jit.run();
        int64_t t_exec = us_since(t0);

        if (opts.benchmark) {
            printBenchmark(t_lang, t_lex, t_parse, t_sem,
                           t_codegen, t_jit, t_exec);
        } else {
            std::cout << "= " << result << "\n";
        }

    } catch (const kem::KemError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error interno: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
