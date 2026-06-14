#include "kem/JITEngine.hpp"

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"

namespace kem {

// Helper: convierte llvm::Error a KemError
static void checkError(llvm::Error err, const std::string& context) {
    if (err) {
        std::string msg;
        llvm::raw_string_ostream ss(msg);
        ss << llvm::toString(std::move(err));
        throw KemError(Phase::JIT, context + ": " + ss.str());
    }
}

// Helper: extrae el valor de un Expected<T> o lanza
template<typename T>
static T unwrap(llvm::Expected<T> val, const std::string& context) {
    if (!val) {
        std::string msg;
        llvm::raw_string_ostream ss(msg);
        ss << llvm::toString(val.takeError());
        throw KemError(Phase::JIT, context + ": " + ss.str());
    }
    return std::move(*val);
}

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
JITEngine::JITEngine() {
    // Inicializar el backend para la CPU actual
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Crear el JIT engine con LLJITBuilder
    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) {
        std::string msg;
        llvm::raw_string_ostream ss(msg);
        ss << llvm::toString(jit.takeError());
        throw KemError(Phase::JIT,
            "No se pudo crear el JIT engine: " + ss.str());
    }

    jit_ = std::move(*jit);
}

// ─────────────────────────────────────────────
//  addModule
// ─────────────────────────────────────────────
void JITEngine::addModule(std::unique_ptr<llvm::Module> module,
                           llvm::LLVMContext& ctx) {
    // ThreadSafeModule envuelve el módulo con su contexto
    // para compilación thread-safe
    auto tsm = llvm::orc::ThreadSafeModule(
        std::move(module),
        std::make_unique<llvm::LLVMContext>()
    );

    checkError(
        jit_->addIRModule(std::move(tsm)),
        "No se pudo agregar el módulo al JIT"
    );
}

// ─────────────────────────────────────────────
//  run
// ─────────────────────────────────────────────
int64_t JITEngine::run() {
    // Buscar el símbolo "inicio" en el módulo compilado
    auto sym = jit_->lookup("inicio");
    if (!sym) {
        std::string msg;
        llvm::raw_string_ostream ss(msg);
        ss << llvm::toString(sym.takeError());
        throw KemError(Phase::JIT,
            "No se encontró el bloque 'inicio'. "
            "Asegurate de que el programa tiene un bloque inicio{}. "
            "Detalle: " + ss.str());
    }

    // Castear a puntero de función: int64_t ()
    // El bloque inicio{} se compila como una función que retorna i64
    auto fn_ptr = sym->toPtr<int64_t()>();

    // Ejecutar
    return fn_ptr();
}

} // namespace kem
