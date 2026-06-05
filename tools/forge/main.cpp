#include <cstdio>
#include <cstdlib>
#include <print>
#include <string_view>
#include <vector>

#include "forge/Conversion/ASTToMLIR/MLIRGenerator.h"
#include "forge/Conversion/MLIRToLLVM/Passes.h"
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/Parser.h"
#include "forge/Frontend/SymbolResolver.h"
#include "forge/Frontend/TypeChecker.h"
#include "forge/Target/JIT.h"

#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

static void print_usage(std::string_view name) {
    std::println("Usage: {} [build|test] [--emit-mlir] [--emit-llvm] <filename>", name);
}

int main(int argc, char **argv) {
    bool emit_mlir = false;
    bool emit_llvm = false;
    bool run_tests = false;
    std::vector<std::string_view> positionals;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if (arg == "--emit-mlir") {
            emit_mlir = true;
        } else if (arg == "--emit-llvm") {
            emit_llvm = true;
        } else {
            positionals.push_back(arg);
        }
    }

    // Optional leading subcommand: `build` (default) or `test`. Anything else is
    // treated as the source file, so `forge <file>` keeps working as a build.
    std::string_view source_path;
    size_t file_index = 0;
    if (!positionals.empty() && (positionals[0] == "build" || positionals[0] == "test")) {
        run_tests = (positionals[0] == "test");
        file_index = 1;
    }
    if (file_index < positionals.size()) {
        source_path = positionals[file_index];
    }

    if (source_path.empty()) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    llvm::SourceMgr source_mgr;
    forge::DiagnosticEmitter emitter{source_mgr};

    auto ast = Parser::parse(source_path, source_mgr, emitter);
    if (!ast) {
        return EXIT_FAILURE;
    }

    {
        forge::SymbolResolver resolver{emitter};
        if (!resolver.resolve(*ast)) {
            return EXIT_FAILURE;
        }
    }

    {
        mlir::MLIRContext type_ctx;
        forge::TypeChecker checker{type_ctx, emitter};
        if (!checker.check(*ast)) {
            return EXIT_FAILURE;
        }
    }

    auto mod = MLIRGenerator::compile(std::move(ast), run_tests);

    if (emit_mlir) {
        llvm::outs() << *mod.module_op << '\n';
        llvm::outs().flush();
        return EXIT_SUCCESS;
    }

    if (!forge::lower(mod)) {
        return EXIT_FAILURE;
    }

    if (emit_llvm) {
        llvm::outs() << *mod.module_op << '\n';
        llvm::outs().flush();
        return EXIT_SUCCESS;
    }

    // Make stdout unbuffered so the assertion text is observable before the process aborts.
    std::setbuf(stdout, nullptr);
    return forge::execute(mod) ? EXIT_SUCCESS : EXIT_FAILURE;
}
