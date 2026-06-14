#include "kem/IRGenerator.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

#include <sstream>

namespace kem {

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
IRGenerator::IRGenerator()
    : module_(std::make_unique<llvm::Module>("kem_module", ctx_)),
      builder_(ctx_)
{}

// ─────────────────────────────────────────────
//  Helpers de tipos
// ─────────────────────────────────────────────
llvm::Type* IRGenerator::toLLVMType(TypeKind kind) {
    switch (kind) {
        case TypeKind::ENTERO:   return llvm::Type::getInt64Ty(ctx_);
        case TypeKind::DECIMAL:  return llvm::Type::getDoubleTy(ctx_);
        case TypeKind::BOOLEANO: return llvm::Type::getInt1Ty(ctx_);
        case TypeKind::TEXTO:    return llvm::PointerType::get(
                                     llvm::Type::getInt8Ty(ctx_), 0);
        case TypeKind::VOID:     return llvm::Type::getVoidTy(ctx_);
        default:
            // UNKNOWN se usa para structs de usuario — por ahora i64
            // (el sistema de tipos de usuario se extiende en fase futura)
            return llvm::Type::getInt64Ty(ctx_);
    }
}

llvm::Type* IRGenerator::toLLVMType(const TypeAnnotation& ta) {
    if (ta.is_array) {
        llvm::Type* elem = toLLVMType(ta.kind);
        return llvm::ArrayType::get(elem, static_cast<uint64_t>(ta.array_size));
    }
    return toLLVMType(ta.kind);
}

// ─────────────────────────────────────────────
//  createEntryAlloca
//  Crea la alloca al INICIO del bloque entry de la función.
//  mem2reg requiere que todas las allocas estén en el entry block.
// ─────────────────────────────────────────────
llvm::AllocaInst* IRGenerator::createEntryAlloca(llvm::Function* fn,
                                                   llvm::Type* type,
                                                   const std::string& name) {
    // Crear un builder temporal que apunta al inicio del entry block
    llvm::IRBuilder<> tmp(ctx_);
    llvm::BasicBlock& entry = fn->getEntryBlock();
    tmp.SetInsertPoint(&entry, entry.begin());
    return tmp.CreateAlloca(type, nullptr, name);
}

// ─────────────────────────────────────────────
//  blockHasTerminator
// ─────────────────────────────────────────────
bool IRGenerator::blockHasTerminator() const {
    llvm::BasicBlock* bb = builder_.GetInsertBlock();
    if (!bb) return false;
    return bb->getTerminator() != nullptr;
}

// ─────────────────────────────────────────────
//  takeModule / emitIR
// ─────────────────────────────────────────────
std::unique_ptr<llvm::Module> IRGenerator::takeModule() {
    return std::move(module_);
}

void IRGenerator::emitIR() const {
    module_->print(llvm::outs(), nullptr);
}

// ─────────────────────────────────────────────
//  generate — punto de entrada
// ─────────────────────────────────────────────
void IRGenerator::generate(Program& prog) {
    prog.accept(*this);

    // Verificar que el módulo generado es IR válido
    std::string err;
    llvm::raw_string_ostream es(err);
    if (llvm::verifyModule(*module_, &es)) {
        throw KemError(Phase::CODEGEN,
            "El IR generado contiene errores internos: " + es.str());
    }

    // Correr optimizaciones O1 (incluye mem2reg, constant folding, DCE)
    llvm::LoopAnalysisManager     lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager    cgam;
    llvm::ModuleAnalysisManager   mam;

    llvm::PassBuilder pb;
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    auto mpm = pb.buildPerModuleDefaultPipeline(
        llvm::OptimizationLevel::O1);
    mpm.run(*module_, mam);
}

// ─────────────────────────────────────────────
//  Pasada 1: declarar signaturas de funciones
//  Se hace antes de generar cuerpos para que las
//  llamadas forward (f llama a g definida después) funcionen.
// ─────────────────────────────────────────────
void IRGenerator::declareFunction(FuncDef& fn) {
    std::vector<llvm::Type*> param_types;
    for (auto& p : fn.params) {
        param_types.push_back(toLLVMType(p.type));
    }

    llvm::FunctionType* ft = llvm::FunctionType::get(
        toLLVMType(fn.return_type), param_types, false);

    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, fn.name, module_.get());

    // Nombrar los argumentos para que el IR sea legible
    size_t i = 0;
    for (auto& arg : f->args()) {
        if (i < fn.params.size()) {
            arg.setName(fn.params[i++].name);
        }
    }

    functions_[fn.name] = f;
}

void IRGenerator::declareProcedure(ProcDef& proc) {
    std::vector<llvm::Type*> param_types;
    for (auto& p : proc.params) {
        param_types.push_back(toLLVMType(p.type));
    }

    llvm::FunctionType* ft = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx_), param_types, false);

    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, proc.name, module_.get());

    size_t i = 0;
    for (auto& arg : f->args()) {
        if (i < proc.params.size()) {
            arg.setName(proc.params[i++].name);
        }
    }

    functions_[proc.name] = f;
}

void IRGenerator::declareLink(LinkDecl& lnk) {
    std::vector<llvm::Type*> param_types;
    for (auto& pt : lnk.param_types) {
        param_types.push_back(toLLVMType(pt));
    }

    llvm::FunctionType* ft = llvm::FunctionType::get(
        toLLVMType(lnk.return_type), param_types, false);

    // ExternalLinkage → el JIT busca el símbolo en las librerías del sistema
    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, lnk.name, module_.get());

    functions_[lnk.name] = f;
}

// ─────────────────────────────────────────────
//  visit(Program)
// ─────────────────────────────────────────────
void IRGenerator::visit(Program& prog) {
    // Pasada 1: declarar todas las signaturas
    for (auto& decl : prog.decls) {
        if (auto* fn   = dynamic_cast<FuncDef*>(decl.get()))   declareFunction(*fn);
        else if (auto* p = dynamic_cast<ProcDef*>(decl.get())) declareProcedure(*p);
        else if (auto* l = dynamic_cast<LinkDecl*>(decl.get())) declareLink(*l);
        // StructDef: no genera código IR en esta fase
    }

    // Pasada 2: generar cuerpos
    for (auto& decl : prog.decls) {
        decl->accept(*this);
    }

    // Generar inicio{} como función @inicio que retorna i64
    if (prog.main_block) {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(ctx_), false);
        llvm::Function* main_fn = llvm::Function::Create(
            ft, llvm::Function::ExternalLinkage, "inicio", module_.get());

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(
            ctx_, "entry", main_fn);
        builder_.SetInsertPoint(entry);

        current_fn_ = main_fn;
        named_values_.clear();

        prog.main_block->accept(*this);

        // Si el bloque no terminó con devolver, agregar ret 0
        if (!blockHasTerminator()) {
            builder_.CreateRet(llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(ctx_), 0));
        }

        current_fn_ = nullptr;
    }
}

// ─────────────────────────────────────────────
//  visit(FuncDef)
// ─────────────────────────────────────────────
void IRGenerator::visit(FuncDef& fn) {
    llvm::Function* f = functions_.at(fn.name);
    current_fn_ = f;
    named_values_.clear();

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        ctx_, "entry", f);
    builder_.SetInsertPoint(entry);

    // Crear alloca para cada parámetro y guardar el valor inicial
    size_t i = 0;
    for (auto& arg : f->args()) {
        const Param& p = fn.params[i++];
        llvm::AllocaInst* alloca = createEntryAlloca(
            f, toLLVMType(p.type), p.name);
        builder_.CreateStore(&arg, alloca);
        named_values_[p.name] = alloca;
    }

    // Generar el cuerpo
    fn.body->accept(*this);

    // Ret de seguridad — no debería ejecutarse si el semántico
    // verificó que la función siempre retorna, pero LLVM lo requiere.
    if (!blockHasTerminator()) {
        llvm::Type* ret_t = toLLVMType(fn.return_type);
        if (ret_t->isVoidTy()) {
            builder_.CreateRetVoid();
        } else if (ret_t->isIntegerTy()) {
            builder_.CreateRet(llvm::ConstantInt::get(ret_t, 0));
        } else if (ret_t->isDoubleTy()) {
            builder_.CreateRet(llvm::ConstantFP::get(ret_t, 0.0));
        }
    }

    current_fn_ = nullptr;
}

// ─────────────────────────────────────────────
//  visit(ProcDef)
// ─────────────────────────────────────────────
void IRGenerator::visit(ProcDef& proc) {
    llvm::Function* f = functions_.at(proc.name);
    current_fn_ = f;
    named_values_.clear();

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        ctx_, "entry", f);
    builder_.SetInsertPoint(entry);

    size_t i = 0;
    for (auto& arg : f->args()) {
        const Param& p = proc.params[i++];
        llvm::AllocaInst* alloca = createEntryAlloca(
            f, toLLVMType(p.type), p.name);
        builder_.CreateStore(&arg, alloca);
        named_values_[p.name] = alloca;
    }

    proc.body->accept(*this);

    if (!blockHasTerminator()) {
        builder_.CreateRetVoid();
    }

    current_fn_ = nullptr;
}

// ─────────────────────────────────────────────
//  visit(StructDef)
//  Los structs se manejan como tipos LLVM en fases futuras.
//  Por ahora no generan código.
// ─────────────────────────────────────────────
void IRGenerator::visit(StructDef&) {}

// ─────────────────────────────────────────────
//  visit(LinkDecl)
//  Ya declarado en declareLink — nada más que hacer.
// ─────────────────────────────────────────────
void IRGenerator::visit(LinkDecl&) {}

// ─────────────────────────────────────────────
//  visit(Block)
//  El bloque no crea un nuevo BasicBlock — eso lo hacen
//  las sentencias que lo necesitan (if, while, for).
//  El bloque simplemente visita sus sentencias en orden.
// ─────────────────────────────────────────────
void IRGenerator::visit(Block& block) {
    for (auto& stmt : block.stmts) {
        // Si el bloque actual ya tiene terminador (ej: devolver en un if),
        // no tiene sentido seguir emitiendo instrucciones — código muerto.
        if (blockHasTerminator()) break;
        stmt->accept(*this);
    }
}

// ─────────────────────────────────────────────
//  visit(VarDecl)
//  entero x = expr  →  alloca + store
// ─────────────────────────────────────────────
void IRGenerator::visit(VarDecl& decl) {
    llvm::Type* type = toLLVMType(decl.type);
    llvm::AllocaInst* alloca = createEntryAlloca(
        current_fn_, type, decl.name);
    named_values_[decl.name] = alloca;

    if (decl.init) {
        decl.init->accept(*this);
        llvm::Value* init_val = last_value_;

        // Conversión implícita entero → decimal si es necesario
        if (decl.type.kind == TypeKind::DECIMAL &&
            init_val->getType()->isIntegerTy()) {
            init_val = builder_.CreateSIToFP(
                init_val, llvm::Type::getDoubleTy(ctx_), "cast");
        }

        builder_.CreateStore(init_val, alloca);
    } else {
        // Sin inicializador → zero-initialize
        builder_.CreateStore(llvm::Constant::getNullValue(type), alloca);
    }
}

// ─────────────────────────────────────────────
//  visit(ArrayDecl)
//  entero nums[N] = [e1, e2, ...]  →  alloca [N x i64] + stores
// ─────────────────────────────────────────────
void IRGenerator::visit(ArrayDecl& decl) {
    llvm::Type* arr_type = toLLVMType(decl.type);
    llvm::AllocaInst* alloca = createEntryAlloca(
        current_fn_, arr_type, decl.name);
    named_values_[decl.name] = alloca;

    // Zero-initialize el arreglo completo
    builder_.CreateStore(llvm::Constant::getNullValue(arr_type), alloca);

    // Si hay inicializadores, escribir elemento por elemento
    llvm::Type* elem_type = toLLVMType(decl.type.kind);
    for (size_t i = 0; i < decl.init.size(); ++i) {
        decl.init[i]->accept(*this);
        llvm::Value* val = last_value_;

        // GEP: calcular la dirección del elemento i
        llvm::Value* elem_ptr = builder_.CreateGEP(
            arr_type, alloca,
            {builder_.getInt64(0), builder_.getInt64(static_cast<int64_t>(i))},
            decl.name + "_" + std::to_string(i));

        // Conversión implícita si el elem es entero y el arreglo es decimal
        if (decl.type.kind == TypeKind::DECIMAL &&
            val->getType()->isIntegerTy()) {
            val = builder_.CreateSIToFP(
                val, elem_type, "cast");
        }

        builder_.CreateStore(val, elem_ptr);
    }
}

// ─────────────────────────────────────────────
//  visit(AssignStmt)
//  x = expr  →  store en el alloca de x
// ─────────────────────────────────────────────
void IRGenerator::visit(AssignStmt& stmt) {
    // Generar el valor a asignar
    stmt.value->accept(*this);
    llvm::Value* val = last_value_;

    // Resolver el puntero destino según el tipo de lvalue
    llvm::Value* ptr = nullptr;

    if (auto* ident = dynamic_cast<IdentExpr*>(stmt.target.get())) {
        // Asignación simple: x = expr
        ptr = named_values_.at(ident->name);

        // Conversión implícita
        llvm::AllocaInst* alloca = named_values_.at(ident->name);
        llvm::Type* target_type = alloca->getAllocatedType();
        if (target_type->isDoubleTy() && val->getType()->isIntegerTy()) {
            val = builder_.CreateSIToFP(
                val, llvm::Type::getDoubleTy(ctx_), "cast");
        }
        ptr = alloca;

    } else if (auto* idx = dynamic_cast<IndexExpr*>(stmt.target.get())) {
        // Asignación de arreglo: arr[i] = expr
        auto* arr_ident = dynamic_cast<IdentExpr*>(idx->object.get());
        llvm::AllocaInst* arr_alloca = named_values_.at(arr_ident->name);
        llvm::Type* arr_type = arr_alloca->getAllocatedType();

        idx->index->accept(*this);
        llvm::Value* index = last_value_;

        ptr = builder_.CreateGEP(
            arr_type, arr_alloca,
            {builder_.getInt64(0), index},
            "elem_ptr");

    } else if (auto* mem = dynamic_cast<MemberExpr*>(stmt.target.get())) {
        // Asignación de miembro de struct: obj.campo = expr
        // Por ahora emite un GEP índice 0 como placeholder —
        // el sistema de tipos de struct se completa en fase futura
        auto* obj_ident = dynamic_cast<IdentExpr*>(mem->object.get());
        if (obj_ident) {
            ptr = named_values_.count(obj_ident->name)
                ? named_values_.at(obj_ident->name)
                : nullptr;
        }
    }

    if (ptr) {
        builder_.CreateStore(val, ptr);
    } else {
        codegenError("No se pudo resolver el destino de la asignación",
                     stmt.line, stmt.col);
    }
}

// ─────────────────────────────────────────────
//  visit(IfStmt)
//  si cond { then } [sino { else }]
//  → 3 bloques: si_true, si_false, merge
// ─────────────────────────────────────────────
void IRGenerator::visit(IfStmt& stmt) {
    // Generar la condición
    stmt.condition->accept(*this);
    llvm::Value* cond = last_value_;

    // Crear los bloques
    llvm::BasicBlock* then_bb  = llvm::BasicBlock::Create(
        ctx_, "si_true",  current_fn_);
    llvm::BasicBlock* else_bb  = llvm::BasicBlock::Create(
        ctx_, "si_false", current_fn_);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(
        ctx_, "si_merge", current_fn_);

    // Salto condicional
    builder_.CreateCondBr(cond, then_bb, else_bb);

    // ── Bloque then ────────────────────────────
    builder_.SetInsertPoint(then_bb);
    stmt.then_block->accept(*this);
    if (!blockHasTerminator()) {
        builder_.CreateBr(merge_bb);
    }

    // ── Bloque else ────────────────────────────
    builder_.SetInsertPoint(else_bb);
    if (stmt.else_block) {
        stmt.else_block->accept(*this);
    }
    if (!blockHasTerminator()) {
        builder_.CreateBr(merge_bb);
    }

    // ── Continuar en merge ─────────────────────
    builder_.SetInsertPoint(merge_bb);
}

// ─────────────────────────────────────────────
//  visit(WhileStmt)
//  mientras cond { body }
//  → 3 bloques: loop_cond, loop_body, loop_exit
// ─────────────────────────────────────────────
void IRGenerator::visit(WhileStmt& stmt) {
    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(
        ctx_, "loop_cond", current_fn_);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(
        ctx_, "loop_body", current_fn_);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(
        ctx_, "loop_exit", current_fn_);

    // Saltar al bloque de condición
    builder_.CreateBr(cond_bb);

    // ── Condición ──────────────────────────────
    builder_.SetInsertPoint(cond_bb);
    stmt.condition->accept(*this);
    llvm::Value* cond = last_value_;
    builder_.CreateCondBr(cond, body_bb, exit_bb);

    // ── Cuerpo ─────────────────────────────────
    builder_.SetInsertPoint(body_bb);
    stmt.body->accept(*this);
    if (!blockHasTerminator()) {
        builder_.CreateBr(cond_bb); // volver a evaluar la condición
    }

    // ── Salida ─────────────────────────────────
    builder_.SetInsertPoint(exit_bb);
}

// ─────────────────────────────────────────────
//  visit(ForStmt)
//  var = inicio hasta fin [paso n] { body }
//  → 4 bloques: for_init, for_cond, for_body, for_exit
// ─────────────────────────────────────────────
void IRGenerator::visit(ForStmt& stmt) {
    llvm::AllocaInst* iter_alloca = named_values_.at(stmt.iter_var);

    // ── Inicializar la variable de iteración ───
    stmt.start->accept(*this);
    builder_.CreateStore(last_value_, iter_alloca);

    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(
        ctx_, "for_cond", current_fn_);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(
        ctx_, "for_body", current_fn_);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(
        ctx_, "for_exit", current_fn_);

    builder_.CreateBr(cond_bb);

    // ── Condición: iter < fin ──────────────────
    builder_.SetInsertPoint(cond_bb);
    llvm::Value* iter_val = builder_.CreateLoad(
        llvm::Type::getInt64Ty(ctx_), iter_alloca, stmt.iter_var);
    stmt.end->accept(*this);
    llvm::Value* end_val = last_value_;
    llvm::Value* cond = builder_.CreateICmpSLT(iter_val, end_val, "for_cond");
    builder_.CreateCondBr(cond, body_bb, exit_bb);

    // ── Cuerpo ─────────────────────────────────
    builder_.SetInsertPoint(body_bb);
    stmt.body->accept(*this);

    // Incrementar la variable de iteración
    if (!blockHasTerminator()) {
        llvm::Value* cur_val = builder_.CreateLoad(
            llvm::Type::getInt64Ty(ctx_), iter_alloca, "iter_cur");

        llvm::Value* step_val;
        if (stmt.step) {
            stmt.step->accept(*this);
            step_val = last_value_;
        } else {
            step_val = llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(ctx_), 1);
        }

        llvm::Value* next_val = builder_.CreateAdd(
            cur_val, step_val, "iter_next");
        builder_.CreateStore(next_val, iter_alloca);
        builder_.CreateBr(cond_bb);
    }

    // ── Salida ─────────────────────────────────
    builder_.SetInsertPoint(exit_bb);
}

// ─────────────────────────────────────────────
//  visit(ReturnStmt)
//  devolver [expr]
// ─────────────────────────────────────────────
void IRGenerator::visit(ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
        llvm::Value* val = last_value_;

        // Conversión implícita si el tipo de retorno es decimal
        llvm::Type* ret_type = current_fn_->getReturnType();
        if (ret_type->isDoubleTy() && val->getType()->isIntegerTy()) {
            val = builder_.CreateSIToFP(
                val, llvm::Type::getDoubleTy(ctx_), "ret_cast");
        }

        builder_.CreateRet(val);
    } else {
        builder_.CreateRetVoid();
    }
}

// ─────────────────────────────────────────────
//  Expresiones — literales
// ─────────────────────────────────────────────
void IRGenerator::visit(NumberLiteral& lit) {
    last_value_ = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(ctx_),
        static_cast<uint64_t>(lit.value),
        true /* signed */);
}

void IRGenerator::visit(FloatLiteral& lit) {
    last_value_ = llvm::ConstantFP::get(
        llvm::Type::getDoubleTy(ctx_), lit.value);
}

void IRGenerator::visit(BoolLiteral& lit) {
    last_value_ = llvm::ConstantInt::get(
        llvm::Type::getInt1Ty(ctx_), lit.value ? 1 : 0);
}

void IRGenerator::visit(StringLiteral& lit) {
    // Crear una cadena global constante y retornar el puntero
    last_value_ = builder_.CreateGlobalStringPtr(lit.value, "str");
}

// ─────────────────────────────────────────────
//  visit(IdentExpr)
//  nombre de variable → load desde su alloca
// ─────────────────────────────────────────────
void IRGenerator::visit(IdentExpr& expr) {
    auto it = named_values_.find(expr.name);
    if (it == named_values_.end()) {
        codegenError("Variable '" + expr.name + "' no encontrada en la tabla de símbolos",
                     expr.line, expr.col);
    }
    llvm::AllocaInst* alloca = it->second;
    last_value_ = builder_.CreateLoad(
        alloca->getAllocatedType(), alloca, expr.name);
}

// ─────────────────────────────────────────────
//  visit(BinaryExpr)
//  Emite la instrucción IR correspondiente al operador.
//  Maneja conversión implícita entero/decimal.
// ─────────────────────────────────────────────
void IRGenerator::visit(BinaryExpr& expr) {
    expr.left->accept(*this);
    llvm::Value* L = last_value_;

    expr.right->accept(*this);
    llvm::Value* R = last_value_;

    // Conversión implícita: si uno es double y el otro int, promover
    bool L_float = L->getType()->isDoubleTy();
    bool R_float = R->getType()->isDoubleTy();

    if (L_float && !R_float) {
        R = builder_.CreateSIToFP(R, llvm::Type::getDoubleTy(ctx_), "cast");
    } else if (!L_float && R_float) {
        L = builder_.CreateSIToFP(L, llvm::Type::getDoubleTy(ctx_), "cast");
    }

    bool is_float = L->getType()->isDoubleTy();

    // Operadores aritméticos
    if (expr.op == "+") { last_value_ = is_float ? builder_.CreateFAdd(L,R,"fadd") : builder_.CreateAdd(L,R,"add"); return; }
    if (expr.op == "-") { last_value_ = is_float ? builder_.CreateFSub(L,R,"fsub") : builder_.CreateSub(L,R,"sub"); return; }
    if (expr.op == "*") { last_value_ = is_float ? builder_.CreateFMul(L,R,"fmul") : builder_.CreateMul(L,R,"mul"); return; }
    if (expr.op == "/") { last_value_ = is_float ? builder_.CreateFDiv(L,R,"fdiv") : builder_.CreateSDiv(L,R,"sdiv"); return; }
    if (expr.op == "%") { last_value_ = builder_.CreateSRem(L,R,"srem"); return; }

    // Operadores relacionales
    if (expr.op == "==") { last_value_ = is_float ? builder_.CreateFCmpOEQ(L,R,"feq") : builder_.CreateICmpEQ(L,R,"eq"); return; }
    if (expr.op == "!=") { last_value_ = is_float ? builder_.CreateFCmpONE(L,R,"fne") : builder_.CreateICmpNE(L,R,"ne"); return; }
    if (expr.op == "<")  { last_value_ = is_float ? builder_.CreateFCmpOLT(L,R,"flt") : builder_.CreateICmpSLT(L,R,"slt"); return; }
    if (expr.op == ">")  { last_value_ = is_float ? builder_.CreateFCmpOGT(L,R,"fgt") : builder_.CreateICmpSGT(L,R,"sgt"); return; }
    if (expr.op == "<=") { last_value_ = is_float ? builder_.CreateFCmpOLE(L,R,"fle") : builder_.CreateICmpSLE(L,R,"sle"); return; }
    if (expr.op == ">=") { last_value_ = is_float ? builder_.CreateFCmpOGE(L,R,"fge") : builder_.CreateICmpSGE(L,R,"sge"); return; }

    // Operadores lógicos (sobre i1)
    if (expr.op == "y") { last_value_ = builder_.CreateAnd(L,R,"and"); return; }
    if (expr.op == "o") { last_value_ = builder_.CreateOr(L,R,"or");   return; }

    codegenError("Operador binario desconocido: '" + expr.op + "'",
                 expr.line, expr.col);
}

// ─────────────────────────────────────────────
//  visit(UnaryExpr)
//  no expr  →  xor con 1
//  -expr    →  neg o fneg
// ─────────────────────────────────────────────
void IRGenerator::visit(UnaryExpr& expr) {
    expr.operand->accept(*this);
    llvm::Value* val = last_value_;

    if (expr.op == "no") {
        // XOR con 1 invierte un i1
        last_value_ = builder_.CreateXor(
            val, llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx_), 1), "not");
        return;
    }
    if (expr.op == "-") {
        last_value_ = val->getType()->isDoubleTy()
            ? builder_.CreateFNeg(val, "fneg")
            : builder_.CreateNeg(val, "neg");
        return;
    }
    codegenError("Operador unario desconocido: '" + expr.op + "'",
                 expr.line, expr.col);
}

// ─────────────────────────────────────────────
//  visit(CallExpr)
//  nombre(arg1, arg2, ...)  →  call
// ─────────────────────────────────────────────
void IRGenerator::visit(CallExpr& expr) {
    auto it = functions_.find(expr.callee);
    if (it == functions_.end()) {
        codegenError("Función '" + expr.callee + "' no encontrada",
                     expr.line, expr.col);
    }
    llvm::Function* callee = it->second;

    std::vector<llvm::Value*> args;
    size_t i = 0;
    for (auto& arg_node : expr.args) {
        arg_node->accept(*this);
        llvm::Value* arg = last_value_;

        // Conversión implícita si el parámetro espera double y recibimos int
        if (i < callee->arg_size()) {
            llvm::Argument* param = callee->getArg(static_cast<unsigned>(i));
            if (param->getType()->isDoubleTy() && arg->getType()->isIntegerTy()) {
                arg = builder_.CreateSIToFP(
                    arg, llvm::Type::getDoubleTy(ctx_), "arg_cast");
            }
        }

        args.push_back(arg);
        ++i;
    }

    if (callee->getReturnType()->isVoidTy()) {
        builder_.CreateCall(callee, args);
        last_value_ = nullptr;
    } else {
        last_value_ = builder_.CreateCall(callee, args, expr.callee + "_ret");
    }
}

// ─────────────────────────────────────────────
//  visit(IndexExpr)
//  arr[i]  →  GEP + load
// ─────────────────────────────────────────────
void IRGenerator::visit(IndexExpr& expr) {
    // Obtener el alloca del arreglo
    auto* arr_ident = dynamic_cast<IdentExpr*>(expr.object.get());
    if (!arr_ident) {
        codegenError("Expresión de arreglo no soportada", expr.line, expr.col);
    }

    llvm::AllocaInst* arr_alloca = named_values_.at(arr_ident->name);
    llvm::Type* arr_type = arr_alloca->getAllocatedType();

    // Calcular el índice
    expr.index->accept(*this);
    llvm::Value* index = last_value_;

    // GEP: arr_ptr, 0 (primer nivel del array), i (índice)
    llvm::Value* elem_ptr = builder_.CreateGEP(
        arr_type, arr_alloca,
        {builder_.getInt64(0), index},
        arr_ident->name + "_idx");

    // Load del elemento
    llvm::Type* elem_type = llvm::cast<llvm::ArrayType>(arr_type)->getElementType();
    last_value_ = builder_.CreateLoad(elem_type, elem_ptr, "elem");
}

// ─────────────────────────────────────────────
//  visit(MemberExpr)
//  obj.campo → placeholder hasta fase de structs completa
// ─────────────────────────────────────────────
void IRGenerator::visit(MemberExpr& expr) {
    // El acceso a miembros de struct se implementa completamente
    // cuando TypeAnnotation soporte tipos de usuario.
    // Por ahora: cargar el puntero base del objeto.
    expr.object->accept(*this);
    // last_value_ queda con el valor del objeto — el campo no se resuelve aún.
}

} // namespace kem
