#include "kem/SemanticAnalyzer.hpp"
#include "kem/ErrorMessages.hpp"
#include <sstream>
#include <iostream>

namespace kem {

// ─────────────────────────────────────────────
//  Scope
// ─────────────────────────────────────────────
Symbol* Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return &it->second;
    if (parent) return parent->lookup(name);
    return nullptr;
}

Symbol* Scope::lookupLocal(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return &it->second;
    return nullptr;
}

void Scope::declare(Symbol sym, int line, int col) {
    if (lookupLocal(sym.name)) {
        throw KemError(Phase::SEMANTIC,
            errorMessages().format("SEM_VAR_DUPLICADA", {sym.name}), line, col);
    }
    symbols[sym.name] = std::move(sym);
}

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
SemanticAnalyzer::SemanticAnalyzer() {
    auto root = std::make_unique<Scope>();
    global_scope_  = root.get();
    current_scope_ = root.get();
    scope_stack_.push_back(std::move(root));
}

// ─────────────────────────────────────────────
//  Scopes
// ─────────────────────────────────────────────
void SemanticAnalyzer::pushScope() {
    auto child   = std::make_unique<Scope>();
    child->parent = current_scope_;
    current_scope_ = child.get();
    scope_stack_.push_back(std::move(child));
}

void SemanticAnalyzer::popScope() {
    if (scope_stack_.size() > 1) {
        scope_stack_.pop_back();
        current_scope_ = scope_stack_.back().get();
    }
}

// ─────────────────────────────────────────────
//  Errores
// ─────────────────────────────────────────────
void SemanticAnalyzer::error(const std::string& msg, int line, int col) {
    ++errors_;
    std::cerr << KemError(Phase::SEMANTIC, msg, line, col).what() << "\n";
}

// ─────────────────────────────────────────────
//  Tipos
// ─────────────────────────────────────────────
bool SemanticAnalyzer::isNumeric(TypeKind k) const {
    return k == TypeKind::ENTERO || k == TypeKind::DECIMAL;
}

bool SemanticAnalyzer::typesCompatible(TypeKind a, TypeKind b) const {
    if (a == b) return true;
    if (isNumeric(a) && isNumeric(b)) return true;
    return false;
}

TypeKind SemanticAnalyzer::binaryResultType(const std::string& op,
                                              TypeKind left, TypeKind right,
                                              int line, int col) {
    if (op == "y" || op == "o") {
        if (left != TypeKind::BOOLEANO || right != TypeKind::BOOLEANO)
            error(errorMessages().format("SEM_OPERADOR_BOOLEANO"), line, col);
        return TypeKind::BOOLEANO;
    }
    if (op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=") {
        if (!typesCompatible(left, right))
            error(errorMessages().format("SEM_COMPARACION_INCOMPATIBLE", {op}), line, col);
        return TypeKind::BOOLEANO;
    }
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        if (!isNumeric(left) || !isNumeric(right)) {
            error(errorMessages().format("SEM_OPERADOR_NUMERICO", {op}), line, col);
            return TypeKind::UNKNOWN;
        }
        if (left == TypeKind::DECIMAL || right == TypeKind::DECIMAL)
            return TypeKind::DECIMAL;
        return TypeKind::ENTERO;
    }
    return TypeKind::UNKNOWN;
}

// ─────────────────────────────────────────────
//  Punto de entrada
// ─────────────────────────────────────────────
void SemanticAnalyzer::analyze(Program& prog) {
    prog.accept(*this);
    if (errors_ > 0) {
        throw KemError(Phase::SEMANTIC,
            errorMessages().format("SEM_ERRORES_ACUMULADOS",
                {std::to_string(errors_)}));
    }
}

// ─────────────────────────────────────────────
//  Program — dos pasadas
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(Program& prog) {
    // Pasada 1: registrar signaturas (forward declarations)
    for (auto& decl : prog.decls) {
        try {
            if (auto* fn = dynamic_cast<FuncDef*>(decl.get())) {
                Symbol sym;
                sym.kind = Symbol::Kind::FUNCTION;
                sym.name = fn->name; sym.return_type = fn->return_type;
                sym.params = fn->params; sym.type = fn->return_type;
                sym.line = fn->line; sym.col = fn->col;
                current_scope_->declare(std::move(sym), fn->line, fn->col);
            } else if (auto* proc = dynamic_cast<ProcDef*>(decl.get())) {
                Symbol sym;
                sym.kind = Symbol::Kind::PROCEDURE;
                sym.name = proc->name; sym.params = proc->params;
                sym.type.kind = TypeKind::VOID;
                sym.line = proc->line; sym.col = proc->col;
                current_scope_->declare(std::move(sym), proc->line, proc->col);
            } else if (auto* st = dynamic_cast<StructDef*>(decl.get())) {
                Symbol sym;
                sym.kind = Symbol::Kind::STRUCT;
                sym.name = st->name; sym.fields = st->fields;
                sym.type.kind = TypeKind::UNKNOWN;
                sym.line = st->line; sym.col = st->col;
                current_scope_->declare(std::move(sym), st->line, st->col);
            } else if (auto* lnk = dynamic_cast<LinkDecl*>(decl.get())) {
                Symbol sym;
                sym.kind = Symbol::Kind::FUNCTION;
                sym.name = lnk->name; sym.return_type = lnk->return_type;
                sym.type = lnk->return_type;
                sym.line = lnk->line; sym.col = lnk->col;
                for (auto& pt : lnk->param_types) {
                    Param p; p.type = pt; p.name = "_";
                    sym.params.push_back(p);
                }
                current_scope_->declare(std::move(sym), lnk->line, lnk->col);
            }
        } catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }

    // Pasada 2: analizar cuerpos
    for (auto& decl : prog.decls) {
        try { decl->accept(*this); }
        catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }
    if (prog.main_block) {
        try { prog.main_block->accept(*this); }
        catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }
}

// ─────────────────────────────────────────────
//  FuncDef
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(FuncDef& fn) {
    pushScope();
    for (auto& p : fn.params) {
        Symbol sym;
        sym.kind = Symbol::Kind::PARAM; sym.name = p.name;
        sym.type = p.type; sym.line = p.line; sym.col = p.col;
        try { current_scope_->declare(std::move(sym), p.line, p.col); }
        catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }

    bool prev_in = in_function_; TypeKind prev_ret = current_return_type_;
    bool prev_found = found_return_; std::string prev_name = current_func_name_;

    in_function_ = true; current_return_type_ = fn.return_type.kind;
    found_return_ = false; current_func_name_ = fn.name;

    fn.body->accept(*this);

    if (!found_return_)
        error(errorMessages().format("SEM_FUNCION_NO_RETORNA", {fn.name}), fn.line, fn.col);

    in_function_ = prev_in; current_return_type_ = prev_ret;
    found_return_ = prev_found; current_func_name_ = prev_name;
    popScope();
}

// ─────────────────────────────────────────────
//  ProcDef
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(ProcDef& proc) {
    pushScope();
    for (auto& p : proc.params) {
        Symbol sym;
        sym.kind = Symbol::Kind::PARAM; sym.name = p.name;
        sym.type = p.type; sym.line = p.line; sym.col = p.col;
        try { current_scope_->declare(std::move(sym), p.line, p.col); }
        catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }
    bool prev_in = in_function_; TypeKind prev_ret = current_return_type_;
    std::string prev_name = current_func_name_;
    in_function_ = false; current_return_type_ = TypeKind::VOID;
    current_func_name_ = proc.name;
    proc.body->accept(*this);
    in_function_ = prev_in; current_return_type_ = prev_ret;
    current_func_name_ = prev_name;
    popScope();
}

// ─────────────────────────────────────────────
//  StructDef
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(StructDef& st) {
    std::unordered_map<std::string, bool> seen;
    for (auto& f : st.fields) {
        if (seen.count(f.name))
            error(errorMessages().format("SEM_CAMPO_DUPLICADO", {f.name, st.name}),
                  f.line, f.col);
        seen[f.name] = true;
    }
}

void SemanticAnalyzer::visit(LinkDecl&) {}

// ─────────────────────────────────────────────
//  Block
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(Block& block) {
    pushScope();
    for (auto& stmt : block.stmts) {
        try { stmt->accept(*this); }
        catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
    }
    popScope();
}

// ─────────────────────────────────────────────
//  VarDecl
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(VarDecl& decl) {
    if (decl.init) {
        decl.init->accept(*this);
        if (last_type_.kind != TypeKind::UNKNOWN &&
            decl.type.kind != TypeKind::UNKNOWN &&
            !typesCompatible(decl.type.kind, last_type_.kind)) {
            error(errorMessages().format("SEM_TIPO_INCOMPATIBLE_INIT",
                  {decl.name, decl.type.toString(),
                   TypeAnnotation{last_type_.kind,false,0}.toString()}),
                  decl.line, decl.col);
        }
    }
    Symbol sym;
    sym.kind = Symbol::Kind::VARIABLE; sym.name = decl.name;
    sym.type = decl.type; sym.line = decl.line; sym.col = decl.col;
    try { current_scope_->declare(std::move(sym), decl.line, decl.col); }
    catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
}

// ─────────────────────────────────────────────
//  ArrayDecl
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(ArrayDecl& decl) {
    if (decl.type.array_size <= 0)
        error(errorMessages().format("SEM_ARREGLO_TAMANO_INVALIDO", {decl.name}),
              decl.line, decl.col);

    if (!decl.init.empty()) {
        if ((int)decl.init.size() > decl.type.array_size)
            error(errorMessages().format("SEM_ARREGLO_INIT_EXCEDE", {decl.name}),
                  decl.line, decl.col);
        for (auto& elem : decl.init) {
            elem->accept(*this);
            if (last_type_.kind != TypeKind::UNKNOWN &&
                !typesCompatible(decl.type.kind, last_type_.kind))
                error(errorMessages().format("SEM_ARREGLO_INIT_TIPO", {decl.name}),
                      decl.line, decl.col);
        }
    }
    Symbol sym;
    sym.kind = Symbol::Kind::ARRAY; sym.name = decl.name;
    sym.type = decl.type; sym.line = decl.line; sym.col = decl.col;
    try { current_scope_->declare(std::move(sym), decl.line, decl.col); }
    catch (const KemError& e) { error(e.message(), e.line(), e.col()); }
}

// ─────────────────────────────────────────────
//  AssignStmt
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(AssignStmt& stmt) {
    stmt.value->accept(*this);
    TypeKind val_type = last_type_.kind;
    stmt.target->accept(*this);
    TypeKind tgt_type = last_type_.kind;

    if (tgt_type != TypeKind::UNKNOWN && val_type != TypeKind::UNKNOWN &&
        !typesCompatible(tgt_type, val_type)) {
        error(errorMessages().format("SEM_TIPO_INCOMPATIBLE_ASIGN",
              {TypeAnnotation{val_type,false,0}.toString(),
               TypeAnnotation{tgt_type,false,0}.toString()}),
              stmt.line, stmt.col);
    }
}

// ─────────────────────────────────────────────
//  IfStmt / WhileStmt / ForStmt / ReturnStmt
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(IfStmt& stmt) {
    stmt.condition->accept(*this);
    if (last_type_.kind != TypeKind::BOOLEANO && last_type_.kind != TypeKind::UNKNOWN)
        error(errorMessages().format("SEM_SI_CONDICION_BOOLEANA"), stmt.line, stmt.col);
    stmt.then_block->accept(*this);
    if (stmt.else_block) stmt.else_block->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt& stmt) {
    stmt.condition->accept(*this);
    if (last_type_.kind != TypeKind::BOOLEANO && last_type_.kind != TypeKind::UNKNOWN)
        error(errorMessages().format("SEM_MIENTRAS_CONDICION_BOOLEANA"), stmt.line, stmt.col);
    stmt.body->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt& stmt) {
    Symbol* sym = current_scope_->lookup(stmt.iter_var);
    if (!sym)
        error(errorMessages().format("SEM_ITER_VAR_NO_DECLARADA", {stmt.iter_var}),
              stmt.line, stmt.col);
    else if (sym->type.kind != TypeKind::ENTERO)
        error(errorMessages().format("SEM_ITER_VAR_NO_ENTERO", {stmt.iter_var}),
              stmt.line, stmt.col);

    stmt.start->accept(*this);
    stmt.end->accept(*this);
    if (stmt.step) stmt.step->accept(*this);
    stmt.body->accept(*this);
}

void SemanticAnalyzer::visit(ReturnStmt& stmt) {
    if (stmt.value) {
        if (!in_function_) {
            error(errorMessages().format("SEM_PROC_NO_RETORNA_VALOR"), stmt.line, stmt.col);
            return;
        }
        stmt.value->accept(*this);
        if (last_type_.kind != TypeKind::UNKNOWN &&
            !typesCompatible(current_return_type_, last_type_.kind)) {
            error(errorMessages().format("SEM_RETORNO_TIPO_INCORRECTO",
                  {current_func_name_,
                   TypeAnnotation{current_return_type_,false,0}.toString(),
                   TypeAnnotation{last_type_.kind,false,0}.toString()}),
                  stmt.line, stmt.col);
        }
        found_return_ = true;
    }
}

// ─────────────────────────────────────────────
//  Expresiones
// ─────────────────────────────────────────────
void SemanticAnalyzer::visit(NumberLiteral&)  { last_type_ = {TypeKind::ENTERO,   false, 0}; }
void SemanticAnalyzer::visit(FloatLiteral&)   { last_type_ = {TypeKind::DECIMAL,  false, 0}; }
void SemanticAnalyzer::visit(StringLiteral&)  { last_type_ = {TypeKind::TEXTO,    false, 0}; }
void SemanticAnalyzer::visit(BoolLiteral&)    { last_type_ = {TypeKind::BOOLEANO, false, 0}; }

void SemanticAnalyzer::visit(IdentExpr& expr) {
    Symbol* sym = current_scope_->lookup(expr.name);
    if (!sym) {
        error(errorMessages().format("SEM_VAR_NO_DECLARADA", {expr.name}), expr.line, expr.col);
        last_type_ = {TypeKind::UNKNOWN, false, 0};
        return;
    }
    last_type_ = sym->type;
}

void SemanticAnalyzer::visit(BinaryExpr& expr) {
    expr.left->accept(*this);  TypeKind left  = last_type_.kind;
    expr.right->accept(*this); TypeKind right = last_type_.kind;
    last_type_ = {binaryResultType(expr.op, left, right, expr.line, expr.col), false, 0};
}

void SemanticAnalyzer::visit(UnaryExpr& expr) {
    expr.operand->accept(*this);
    if (expr.op == "no") {
        if (last_type_.kind != TypeKind::BOOLEANO && last_type_.kind != TypeKind::UNKNOWN)
            error(errorMessages().format("SEM_NOT_REQUIERE_BOOLEANO"), expr.line, expr.col);
        last_type_ = {TypeKind::BOOLEANO, false, 0};
    } else if (expr.op == "-") {
        if (!isNumeric(last_type_.kind) && last_type_.kind != TypeKind::UNKNOWN)
            error(errorMessages().format("SEM_NEG_REQUIERE_NUMERICO"), expr.line, expr.col);
    }
}

// ─────────────────────────────────────────────
//  Builtins del lenguaje KEM
//  Estas funciones son nativas — no requieren enlazar ni declarar.
//  El semántico las reconoce por nombre y verifica sus argumentos.
//  El IRGenerator las emite directamente en LLVM IR.
//
//  imprimir(texto)        → void   imprime sin newline
//  imprimirLinea(texto)   → void   imprime con newline al final
//  imprimirEntero(entero) → void   imprime un entero como decimal
//  imprimirDecimal(decimal)→ void  imprime un decimal
//  leerLinea()            → texto  lee una línea de stdin
//  leerEntero()           → entero lee un entero de stdin
//  leerDecimal()          → decimal lee un decimal de stdin
// ─────────────────────────────────────────────
static bool isBuiltin(const std::string& name) {
    return name == "imprimir"        ||
           name == "imprimirLinea"   ||
           name == "imprimirEntero"  ||
           name == "imprimirDecimal" ||
           name == "leerLinea"       ||
           name == "leerEntero"      ||
           name == "leerDecimal";
}

// Verifica argumentos de un builtin y retorna su tipo de resultado
static std::pair<bool, TypeAnnotation> checkBuiltin(
    const std::string& name,
    const std::vector<std::unique_ptr<ASTNode>>& args)
{
    TypeAnnotation void_t  = {TypeKind::VOID,    false, 0};
    TypeAnnotation texto_t = {TypeKind::TEXTO,   false, 0};
    TypeAnnotation int_t   = {TypeKind::ENTERO,  false, 0};
    TypeAnnotation dec_t   = {TypeKind::DECIMAL, false, 0};

    // Funciones de salida — reciben 1 argumento
    if (name == "imprimir" || name == "imprimirLinea") {
        if (args.size() != 1) return {false, void_t};
        return {true, void_t};   // acepta cualquier tipo de arg (texto preferido)
    }
    if (name == "imprimirEntero") {
        if (args.size() != 1) return {false, void_t};
        return {true, void_t};
    }
    if (name == "imprimirDecimal") {
        if (args.size() != 1) return {false, void_t};
        return {true, void_t};
    }
    // Funciones de entrada — sin argumentos
    if (name == "leerLinea")  return {args.empty(), texto_t};
    if (name == "leerEntero") return {args.empty(), int_t};
    if (name == "leerDecimal")return {args.empty(), dec_t};

    return {false, void_t};
}

void SemanticAnalyzer::visit(CallExpr& expr) {
    // ── Builtins: se resuelven antes de buscar en la tabla de símbolos ───────
    if (isBuiltin(expr.callee)) {
        auto [ok, ret_type] = checkBuiltin(expr.callee, expr.args);
        if (!ok) {
            error(errorMessages().format("SEM_BUILTIN_LLAMADA_INVALIDA", {expr.callee}),
                  expr.line, expr.col);
            last_type_ = {TypeKind::UNKNOWN, false, 0};
            return;
        }
        // Verificar tipos de argumentos para los que imprimen
        if (expr.callee == "imprimirEntero" && !expr.args.empty()) {
            expr.args[0]->accept(*this);
            if (last_type_.kind != TypeKind::ENTERO &&
                last_type_.kind != TypeKind::UNKNOWN)
                error(errorMessages().format("SEM_BUILTIN_ESPERA_ENTERO"), expr.line, expr.col);
        } else if (expr.callee == "imprimirDecimal" && !expr.args.empty()) {
            expr.args[0]->accept(*this);
            if (last_type_.kind != TypeKind::DECIMAL &&
                last_type_.kind != TypeKind::UNKNOWN &&
                !isNumeric(last_type_.kind))
                error(errorMessages().format("SEM_BUILTIN_ESPERA_DECIMAL"), expr.line, expr.col);
        } else {
            // imprimir / imprimirLinea: visitar el arg para detectar errores
            for (auto& a : expr.args) a->accept(*this);
        }
        last_type_ = ret_type;
        return;
    }

    // ── Funciones definidas por el usuario ────────────────────────────────────
    Symbol* sym = current_scope_->lookup(expr.callee);
    if (!sym) {
        error(errorMessages().format("SEM_FUNCION_NO_DECLARADA", {expr.callee}),
              expr.line, expr.col);
        last_type_ = {TypeKind::UNKNOWN, false, 0};
        return;
    }
    if (sym->kind != Symbol::Kind::FUNCTION && sym->kind != Symbol::Kind::PROCEDURE) {
        error(errorMessages().format("SEM_NO_ES_FUNCION", {expr.callee}),
              expr.line, expr.col);
        last_type_ = {TypeKind::UNKNOWN, false, 0};
        return;
    }
    if (expr.args.size() != sym->params.size())
        error(errorMessages().format("SEM_ARGS_CANTIDAD",
              {expr.callee, std::to_string(sym->params.size()),
               std::to_string(expr.args.size())}),
              expr.line, expr.col);

    size_t n = std::min(expr.args.size(), sym->params.size());
    for (size_t i = 0; i < n; ++i) {
        expr.args[i]->accept(*this);
        TypeKind arg = last_type_.kind;
        TypeKind par = sym->params[i].type.kind;
        if (arg != TypeKind::UNKNOWN && par != TypeKind::UNKNOWN &&
            !typesCompatible(par, arg))
            error(errorMessages().format("SEM_ARGS_TIPO",
                  {std::to_string(i+1), expr.callee,
                   TypeAnnotation{par,false,0}.toString(),
                   TypeAnnotation{arg,false,0}.toString()}),
                  expr.line, expr.col);
    }
    last_type_ = sym->return_type;
}

void SemanticAnalyzer::visit(IndexExpr& expr) {
    expr.object->accept(*this);
    TypeAnnotation obj = last_type_;
    if (!obj.is_array && obj.kind != TypeKind::UNKNOWN)
        error(errorMessages().format("SEM_INDEXAR_NO_ARREGLO"), expr.line, expr.col);
    expr.index->accept(*this);
    if (last_type_.kind != TypeKind::ENTERO && last_type_.kind != TypeKind::UNKNOWN)
        error(errorMessages().format("SEM_INDICE_NO_ENTERO"), expr.line, expr.col);
    last_type_ = {obj.kind, false, 0};
}

void SemanticAnalyzer::visit(MemberExpr& expr) {
    expr.object->accept(*this);
    // Si el tipo es primitivo, el acceso con '.' es inválido
    if (last_type_.kind != TypeKind::UNKNOWN)
        error(errorMessages().format("SEM_PUNTO_SOLO_ESTRUCTURAS"), expr.line, expr.col);
    // Para tipos de usuario (structs) el Codegen verifica el campo directamente
    last_type_ = {TypeKind::UNKNOWN, false, 0};
}

} // namespace kem
