#pragma once

#include <memory>
#include <string>
#include <vector>
#include "kem/Token.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  Forward declarations para el patrón Visitor
// ─────────────────────────────────────────────
struct NumberLiteral;
struct FloatLiteral;
struct StringLiteral;
struct BoolLiteral;
struct IdentExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct IndexExpr;
struct MemberExpr;
struct AssignStmt;
struct VarDecl;
struct ArrayDecl;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ReturnStmt;
struct Block;
struct FuncDef;
struct ProcDef;
struct StructDef;
struct LinkDecl;
struct Program;

// ─────────────────────────────────────────────
//  Visitor
//  Cada fase que opera sobre el AST es un Visitor:
//  SemanticAnalyzer, IRGenerator, etc.
//  Agregar una fase nueva no requiere tocar los nodos.
// ─────────────────────────────────────────────
struct Visitor {
    virtual ~Visitor() = default;

    virtual void visit(NumberLiteral&)  = 0;
    virtual void visit(FloatLiteral&)   = 0;
    virtual void visit(StringLiteral&)  = 0;
    virtual void visit(BoolLiteral&)    = 0;
    virtual void visit(IdentExpr&)      = 0;
    virtual void visit(BinaryExpr&)     = 0;
    virtual void visit(UnaryExpr&)      = 0;
    virtual void visit(CallExpr&)       = 0;
    virtual void visit(IndexExpr&)      = 0;
    virtual void visit(MemberExpr&)     = 0;
    virtual void visit(AssignStmt&)     = 0;
    virtual void visit(VarDecl&)        = 0;
    virtual void visit(ArrayDecl&)      = 0;
    virtual void visit(IfStmt&)         = 0;
    virtual void visit(WhileStmt&)      = 0;
    virtual void visit(ForStmt&)        = 0;
    virtual void visit(ReturnStmt&)     = 0;
    virtual void visit(Block&)          = 0;
    virtual void visit(FuncDef&)        = 0;
    virtual void visit(ProcDef&)        = 0;
    virtual void visit(StructDef&)      = 0;
    virtual void visit(LinkDecl&)       = 0;
    virtual void visit(Program&)        = 0;
};

// ─────────────────────────────────────────────
//  ASTNode — base de todos los nodos
// ─────────────────────────────────────────────
struct ASTNode {
    int line = 0;
    int col  = 0;

    virtual ~ASTNode() = default;
    virtual void accept(Visitor& v) = 0;
};

using NodePtr = std::unique_ptr<ASTNode>;

// ─────────────────────────────────────────────
//  Tipo representado en el AST
//  Cubre: entero, decimal, texto, booleano
//  y sus variantes en arreglo: entero[N], etc.
// ─────────────────────────────────────────────
enum class TypeKind {
    ENTERO,
    DECIMAL,
    TEXTO,
    BOOLEANO,
    VOID,       // para procedimientos / enlazar void
    UNKNOWN
};

struct TypeAnnotation {
    TypeKind    kind      = TypeKind::UNKNOWN;
    bool        is_array  = false;
    int         array_size = 0;   // 0 = tamaño no especificado

    // Para mensajes de error y emit-ir
    std::string toString() const;
};

// ─────────────────────────────────────────────
//  Expresiones
// ─────────────────────────────────────────────

struct NumberLiteral : ASTNode {
    long long value;
    explicit NumberLiteral(long long v, int ln = 0, int cl = 0)
        : value(v) { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

struct FloatLiteral : ASTNode {
    double value;
    explicit FloatLiteral(double v, int ln = 0, int cl = 0)
        : value(v) { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

struct StringLiteral : ASTNode {
    std::string value;
    explicit StringLiteral(std::string s, int ln = 0, int cl = 0)
        : value(std::move(s)) { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

struct BoolLiteral : ASTNode {
    bool value;
    explicit BoolLiteral(bool v, int ln = 0, int cl = 0)
        : value(v) { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

struct IdentExpr : ASTNode {
    std::string name;
    explicit IdentExpr(std::string n, int ln = 0, int cl = 0)
        : name(std::move(n)) { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Operador binario: +, -, *, /, %, ==, !=, <, >, <=, >=, y, o
struct BinaryExpr : ASTNode {
    std::string op;      // lexema del operador: "+", "==", "y", etc.
    NodePtr     left;
    NodePtr     right;

    BinaryExpr(std::string op, NodePtr l, NodePtr r, int ln = 0, int cl = 0)
        : op(std::move(op)), left(std::move(l)), right(std::move(r))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Operador unario: no, - (negación)
struct UnaryExpr : ASTNode {
    std::string op;     // "no" o "-"
    NodePtr     operand;

    UnaryExpr(std::string op, NodePtr operand, int ln = 0, int cl = 0)
        : op(std::move(op)), operand(std::move(operand))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Llamada a función: nombre(arg1, arg2, ...)
struct CallExpr : ASTNode {
    std::string              callee;
    std::vector<NodePtr>     args;

    CallExpr(std::string callee, std::vector<NodePtr> args, int ln = 0, int cl = 0)
        : callee(std::move(callee)), args(std::move(args))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Acceso a arreglo: nombre[indice]
struct IndexExpr : ASTNode {
    NodePtr object;
    NodePtr index;

    IndexExpr(NodePtr obj, NodePtr idx, int ln = 0, int cl = 0)
        : object(std::move(obj)), index(std::move(idx))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Acceso a campo de estructura: objeto.campo
struct MemberExpr : ASTNode {
    NodePtr     object;
    std::string field;

    MemberExpr(NodePtr obj, std::string field, int ln = 0, int cl = 0)
        : object(std::move(obj)), field(std::move(field))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// ─────────────────────────────────────────────
//  Sentencias
// ─────────────────────────────────────────────

// Asignación: lvalue = expr
// lvalue puede ser IdentExpr, IndexExpr, o MemberExpr
struct AssignStmt : ASTNode {
    NodePtr target;   // el lado izquierdo
    NodePtr value;    // el lado derecho

    AssignStmt(NodePtr target, NodePtr value, int ln = 0, int cl = 0)
        : target(std::move(target)), value(std::move(value))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Declaración de variable escalar: tipo nombre [= expr]
struct VarDecl : ASTNode {
    TypeAnnotation  type;
    std::string     name;
    NodePtr         init;   // puede ser nullptr si no hay inicializador
    bool            is_ref = false; // declarado con 'referencia'

    VarDecl(TypeAnnotation type, std::string name, NodePtr init,
            bool is_ref = false, int ln = 0, int cl = 0)
        : type(type), name(std::move(name)), init(std::move(init)), is_ref(is_ref)
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// Declaración de arreglo: tipo nombre[N] [= [e1, e2, ...]]
struct ArrayDecl : ASTNode {
    TypeAnnotation       type;     // type.is_array = true, type.array_size = N
    std::string          name;
    std::vector<NodePtr> init;     // lista de inicializadores (puede ser vacía)

    ArrayDecl(TypeAnnotation type, std::string name,
              std::vector<NodePtr> init, int ln = 0, int cl = 0)
        : type(type), name(std::move(name)), init(std::move(init))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// si expr { ... } [sino { ... }]
struct IfStmt : ASTNode {
    NodePtr condition;
    NodePtr then_block;
    NodePtr else_block;  // puede ser nullptr, o un IfStmt (sino si)

    IfStmt(NodePtr cond, NodePtr then_b, NodePtr else_b,
           int ln = 0, int cl = 0)
        : condition(std::move(cond)),
          then_block(std::move(then_b)),
          else_block(std::move(else_b))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// mientras expr { ... }
struct WhileStmt : ASTNode {
    NodePtr condition;
    NodePtr body;

    WhileStmt(NodePtr cond, NodePtr body, int ln = 0, int cl = 0)
        : condition(std::move(cond)), body(std::move(body))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// var = inicio hasta fin [paso inc] { ... }
struct ForStmt : ASTNode {
    std::string iter_var;    // nombre de la variable de iteración (ya declarada)
    NodePtr     start;
    NodePtr     end;
    NodePtr     step;        // nullptr si no se especifica (implica paso 1)
    NodePtr     body;

    ForStmt(std::string var, NodePtr start, NodePtr end,
            NodePtr step, NodePtr body, int ln = 0, int cl = 0)
        : iter_var(std::move(var)),
          start(std::move(start)),
          end(std::move(end)),
          step(std::move(step)),
          body(std::move(body))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// devolver [expr]
struct ReturnStmt : ASTNode {
    NodePtr value;  // nullptr en procedimientos con devolver vacío

    explicit ReturnStmt(NodePtr value, int ln = 0, int cl = 0)
        : value(std::move(value))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// { sentencia* }
struct Block : ASTNode {
    std::vector<NodePtr> stmts;

    explicit Block(std::vector<NodePtr> stmts, int ln = 0, int cl = 0)
        : stmts(std::move(stmts))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// ─────────────────────────────────────────────
//  Parámetro de función
// ─────────────────────────────────────────────
struct Param {
    TypeAnnotation type;
    std::string    type_name; // nombre si es tipo de usuario (struct)
    std::string    name;
    bool           is_ref = false;
    int            line   = 0;
    int            col    = 0;
};

// ─────────────────────────────────────────────
//  Declaraciones de nivel superior
// ─────────────────────────────────────────────

// funcion nombre(params) tipo_retorno { body }
struct FuncDef : ASTNode {
    std::string        name;
    std::vector<Param> params;
    TypeAnnotation     return_type;
    NodePtr            body;

    FuncDef(std::string name, std::vector<Param> params,
            TypeAnnotation ret, NodePtr body, int ln = 0, int cl = 0)
        : name(std::move(name)), params(std::move(params)),
          return_type(ret), body(std::move(body))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// procedimiento nombre(params) { body }
struct ProcDef : ASTNode {
    std::string        name;
    std::vector<Param> params;
    NodePtr            body;

    ProcDef(std::string name, std::vector<Param> params,
            NodePtr body, int ln = 0, int cl = 0)
        : name(std::move(name)), params(std::move(params)),
          body(std::move(body))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// estructura Nombre { campo* }
struct StructField {
    TypeAnnotation type;
    std::string    name;
    int            line = 0;
    int            col  = 0;
};

struct StructDef : ASTNode {
    std::string              name;
    std::vector<StructField> fields;

    StructDef(std::string name, std::vector<StructField> fields,
              int ln = 0, int cl = 0)
        : name(std::move(name)), fields(std::move(fields))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// enlazar tipo nombre(tipos_params)
struct LinkDecl : ASTNode {
    TypeAnnotation            return_type;
    std::string               name;
    std::vector<TypeAnnotation> param_types;

    LinkDecl(TypeAnnotation ret, std::string name,
             std::vector<TypeAnnotation> params, int ln = 0, int cl = 0)
        : return_type(ret), name(std::move(name)), param_types(std::move(params))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

// ─────────────────────────────────────────────
//  Raíz del AST
// ─────────────────────────────────────────────
struct Program : ASTNode {
    std::vector<NodePtr> decls;    // funciones, procedimientos, estructuras, enlazar
    NodePtr              main_block; // el bloque inicio { }

    Program(std::vector<NodePtr> decls, NodePtr main_block,
            int ln = 0, int cl = 0)
        : decls(std::move(decls)), main_block(std::move(main_block))
    { line = ln; col = cl; }
    void accept(Visitor& v) override { v.visit(*this); }
};

} // namespace kem
// NOTE: agregado por fix fase-2
// Este archivo fue modificado al final — ver struct Param arriba
// El campo type_name en Param guarda el nombre del tipo cuando
// es un tipo de usuario (struct) en lugar de un tipo primitivo.
// Cuando type.kind == TypeKind::UNKNOWN, usar type_name.
