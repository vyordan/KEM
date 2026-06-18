#pragma once
#include <string>
#include <memory>
#include <future>
#include "kem/LangConfig.hpp"

struct ExecutionResult {
    int  exitCode = 0;
    bool success  = false;
    std::string output;
    std::string errors;
    std::string tokens;
    std::string ast;
    std::string ir;
    std::string benchmark;
};

struct AppState {
    std::string sourceText;
    std::string filePath;
    bool        modified = false;

    std::string langFilePath = std::string(KEM_LANG_DIR) + "/espanol.json";
    std::shared_ptr<kem::LangConfig> langConfig;
    std::string langError;

    bool emitTokens   = false;
    bool emitAST      = false;
    bool emitIR       = false;
    bool benchmark    = false;
    bool captureOutput = false;  // por defecto, no capturar → se ve en la terminal

    ExecutionResult lastResult;
    bool            showResult = false;

    std::future<ExecutionResult> executionFuture;
    bool                         isExecuting = false;
};