#include <cstdlib>
#include <print>
#include <string_view>

#include "forge/Conversion/ASTToMLIR/MLIRGenerator.h"
#include "forge/Conversion/MLIRToLLVM/Passes.h"
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/Parser.h"
#include "forge/Frontend/TypeChecker.h"
#include "forge/Target/JIT.h"

#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

static void print_usage(std::string_view name) {
    std::println("Usage: {} [--emit-mlir] [--emit-llvm] <filename>", name);
}

int main(int argc, char **argv) {
    bool emit_mlir = false;
    bool emit_llvm = false;
    std::string_view source_path;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if (arg == "--emit-mlir")
            emit_mlir = true;
        else if (arg == "--emit-llvm")
            emit_llvm = true;
        else
            source_path = arg;
    }

    if (source_path.empty()) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    llvm::SourceMgr source_mgr;
    forge::DiagnosticEmitter emitter{source_mgr};

    auto ast = Parser::parse(source_path, source_mgr);
    if (!ast)
        return EXIT_FAILURE;

    {
        mlir::MLIRContext type_ctx;
        forge::TypeChecker checker{type_ctx, emitter};
        if (!checker.check(*ast))
            return EXIT_FAILURE;
    }

    auto mod = MLIRGenerator::compile(std::move(ast));

    if (emit_mlir) {
        llvm::outs() << *mod.module_op << '\n';
        llvm::outs().flush();
    }

    if (!forge::lower(mod))
        return EXIT_FAILURE;

    if (emit_llvm) {
        llvm::outs() << *mod.module_op << '\n';
        llvm::outs().flush();
    }

    return forge::execute(mod) ? EXIT_SUCCESS : EXIT_FAILURE;
}
