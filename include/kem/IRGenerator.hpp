#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// LLVM headers — orden importa
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include "kem/AST.hpp"
#include "kem/ErrorHandler.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  IRGenerator
//
//  Visitor sobre el AST. Para cada nodo produce
//  instrucciones LLVM IR a través del IRBuilder.
//
//  Contrato:
//  - El AST ya fue validado por el SemanticAnalyzer.
//    El IRGenerator asume que los tipos son correctos
//    y que todas las variables/funciones están declaradas.
//  - Las expresiones dejan su resultado en last_value_.
//  - Las sentencias modifican el estado del builder
//    (bloques, puntos de inserción, tabla de símbolos).
//  - Al finalizar generate(), el módulo está listo
//    para pasarse al JIT o para emitir IR textual.
// ─────────────────────────────────────────────
class IRGenerator : public Visitor {
public:
    IRGenerator();

    // Punto de entrada. Genera el módulo completo a partir del AST.
    // Verifica el módulo al final. Lanza KemError si el IR es inválido.
    void generate(Program& prog);

    // Transferir la propiedad del módulo al JIT.
    // Solo se puede llamar una vez — después el puntero es nulo.
    std::unique_ptr<llvm::Module> takeModule();

    // Emitir el IR como texto a stdout (para --emit-ir)
    void emitIR() const;

    // Acceso al contexto (el JITEngine lo necesita para ThreadSafeModule)
    llvm::LLVMContext& context() { return ctx_; }

private:
    // ── Pilares de LLVM ────────────────────────
    llvm::LLVMContext              ctx_;
    std::unique_ptr<llvm::Module>  module_;
    llvm::IRBuilder<>              builder_;

    // ── Estado de generación ───────────────────
    llvm::Function* current_fn_ = nullptr;

    // Tabla de símbolos: nombre → alloca ptr
    // Cada función tiene su propio frame — se limpia al salir
    std::unordered_map<std::string, llvm::AllocaInst*> named_values_;

    // Tabla de funciones declaradas: nombre → Function*
    // Incluye funciones de KEM y funciones externas (enlazar)
    std::unordered_map<std::string, llvm::Function*> functions_;

    // Resultado del último nodo de expresión visitado
    llvm::Value* last_value_ = nullptr;

    // ── Helpers de tipos ───────────────────────
    llvm::Type* toLLVMType(TypeKind kind);
    llvm::Type* toLLVMType(const TypeAnnotation& ta);

    // ── Helper crítico: alloca SIEMPRE en el entry block ──
    // mem2reg solo promueve allocas que están en el entry block.
    // Si se pone dentro de un if o loop, no se optimiza.
    llvm::AllocaInst* createEntryAlloca(llvm::Function* fn,
                                         llvm::Type* type,
                                         const std::string& name);

    // ── Helpers de control de flujo ────────────
    // Verifica si el bloque actual ya tiene terminador
    // (ret o br). Si lo tiene, no se puede insertar más.
    bool blockHasTerminator() const;

    // ── Pasada 1: declarar signaturas ──────────
    // Antes de generar cuerpos, declara todas las funciones
    // para que las llamadas forward funcionen.
    void declareFunction(FuncDef& fn);
    void declareProcedure(ProcDef& proc);
    void declareLink(LinkDecl& lnk);

    // ── Visitor — declaraciones ─────────────────
    void visit(Program&)   override;
    void visit(FuncDef&)   override;
    void visit(ProcDef&)   override;
    void visit(StructDef&) override;
    void visit(LinkDecl&)  override;

    // ── Visitor — sentencias ───────────────────
    void visit(Block&)      override;
    void visit(VarDecl&)    override;
    void visit(ArrayDecl&)  override;
    void visit(AssignStmt&) override;
    void visit(IfStmt&)     override;
    void visit(WhileStmt&)  override;
    void visit(ForStmt&)    override;
    void visit(ReturnStmt&) override;

    // ── Visitor — expresiones ──────────────────
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
