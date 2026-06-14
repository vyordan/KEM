#pragma once

#include <memory>
#include <string>
#include <cstdint>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/TargetSelect.h"

#include "kem/ErrorHandler.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  JITEngine
//
//  Wrapper del ORC JIT de LLVM.
//  Recibe el llvm::Module del IRGenerator,
//  lo compila a código nativo para la CPU actual,
//  busca el símbolo "inicio" y lo ejecuta.
//
//  Uso:
//    JITEngine jit;
//    jit.addModule(gen.takeModule(), gen.context());
//    int64_t result = jit.run();
// ─────────────────────────────────────────────
class JITEngine {
public:
    // Inicializa el JIT engine. Llama a InitializeNativeTarget internamente.
    // Lanza KemError si el JIT no se puede crear.
    JITEngine();

    // Agrega el módulo IR al JIT y lo compila.
    // ctx se mueve dentro del ThreadSafeModule — no se usa después.
    void addModule(std::unique_ptr<llvm::Module> module,
                   llvm::LLVMContext& ctx);

    // Busca el símbolo "inicio" en el JIT y lo ejecuta.
    // Retorna el valor de retorno de inicio{}.
    // Lanza KemError si el símbolo no existe o falla la ejecución.
    int64_t run();

private:
    std::unique_ptr<llvm::orc::LLJIT> jit_;
};

} // namespace kem
