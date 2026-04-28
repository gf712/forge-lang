#include "forge/Target/JIT.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <print>

bool forge::execute(forge::CompiledModule &mod) {
    mlir::registerBuiltinDialectTranslation(*mod.ctx);
    mlir::registerLLVMDialectTranslation(*mod.ctx);

    llvm::LLVMContext llvm_context;
    auto llvm_module = mlir::translateModuleToLLVMIR(mod.module_op, llvm_context);
    if (!llvm_module) {
        std::println("Failed to emit LLVM IR");
        return false;
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto maybe_machine_builder = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!maybe_machine_builder) {
        llvm::errs() << "Could not create JITTargetMachineBuilder\n";
        return false;
    }

    auto maybe_target_machine = maybe_machine_builder->createTargetMachine();
    if (!maybe_target_machine) {
        llvm::errs() << "Could not create TargetMachine\n";
        return false;
    }

    mlir::ExecutionEngine::setupTargetTripleAndDataLayout(llvm_module.get(),
                                                          maybe_target_machine.get().get());

    auto opt_pipeline = mlir::makeOptimizingTransformer(0, 0, maybe_target_machine.get().get());
    if (auto err = opt_pipeline(llvm_module.get())) {
        llvm::errs() << "Failed to optimize LLVM IR: " << err << "\n";
        return false;
    }

    auto maybe_engine = mlir::ExecutionEngine::create(mod.module_op.getOperation());
    if (!maybe_engine) {
        llvm::errs() << "Failed to create execution engine\n";
        return false;
    }

    auto result = maybe_engine.get()->invoke("__main");
    if (result) {
        llvm::errs() << "Failed to invoke JIT compiled function: " << result << '\n';
        return false;
    }

    return true;
}
