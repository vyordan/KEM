#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "kem/AST.hpp"
#include "kem/ErrorHandler.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  Symbol — entrada en la tabla de símbolos
// ─────────────────────────────────────────────
struct Symbol {
    enum class Kind { VARIABLE, ARRAY, FUNCTION, PROCEDURE, STRUCT, PARAM };

    Kind           kind;
    TypeAnnotation type;
    std::string    name;
    int            line = 0;
    int            col  = 0;

    // Para funciones/procedimientos
    std::vector<Param> params;
    TypeAnnotation     return_type;

    // Para structs
    std::vector<StructField> fields;
};

// ─────────────────────────────────────────────
//  Scope — tabla de símbolos de un bloque
// ─────────────────────────────────────────────
struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent = nullptr;

    Symbol* lookup(const std::string& name);
    Symbol* lookupLocal(const std::string& name);
    void    declare(Symbol sym, int line, int col);
};

// ─────────────────────────────────────────────
//  SemanticAnalyzer
// ─────────────────────────────────────────────
class SemanticAnalyzer : public Visitor {
public:
    SemanticAnalyzer();

    void   analyze(Program& prog);
    Scope* globalScope() { return global_scope_; }

private:
    // Stack de scopes — cada push crea un scope hijo
    std::vector<std::unique_ptr<Scope>> scope_stack_;
    Scope* global_scope_  = nullptr;
    Scope* current_scope_ = nullptr;

    void pushScope();
    void popScope();

    // Estado de análisis
    int         errors_              = 0;
    TypeKind    current_return_type_ = TypeKind::VOID;
    bool        in_function_         = false;
    bool        found_return_        = false;
    std::string current_func_name_;

    // Tipo del último nodo de expresión visitado
    TypeAnnotation last_type_;

    void error(const std::string& msg, int line, int col);

    // Helpers de tipos
    bool     typesCompatible(TypeKind a, TypeKind b) const;
    bool     isNumeric(TypeKind k) const;
    TypeKind binaryResultType(const std::string& op, TypeKind left, TypeKind right,
                               int line, int col);

    // Visitor — declaraciones
    void visit(Program&)   override;
    void visit(FuncDef&)   override;
    void visit(ProcDef&)   override;
    void visit(StructDef&) override;
    void visit(LinkDecl&)  override;

    // Visitor — sentencias
    void visit(Block&)      override;
    void visit(VarDecl&)    override;
    void visit(ArrayDecl&)  override;
    void visit(AssignStmt&) override;
    void visit(IfStmt&)     override;
    void visit(WhileStmt&)  override;
    void visit(ForStmt&)    override;
    void visit(ReturnStmt&) override;

    // Visitor — expresiones
    void visit(NumberLiteral&)  override;
    void visit(FloatLiteral&)   override;
    void visit(StringLiteral&)  override;
    void visit(BoolLiteral&)    override;
    void visit(IdentExpr&)      override;
    void visit(BinaryExpr&)     override;
    void visit(UnaryExpr&)      override;
    void visit(CallExpr&)       override;
    void visit(IndexExpr&)      override;
    void visit(MemberExpr&)     override;
};

} // namespace kem
