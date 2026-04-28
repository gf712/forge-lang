#include "forge/Conversion/MLIRToLLVM/Passes.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include <print>

bool forge::lower(forge::CompiledModule &mod) {
    mlir::ConversionTarget target(*mod.ctx);
    target.addLegalDialect<mlir::LLVM::LLVMDialect>();
    target.addLegalOp<mlir::ModuleOp>();

    mlir::RewritePatternSet patterns(mod.ctx.get());
    mlir::LLVMTypeConverter type_converter{mod.ctx.get()};
    mlir::arith::populateArithToLLVMConversionPatterns(type_converter, patterns);
    mlir::populateFuncToLLVMConversionPatterns(type_converter, patterns);

    mlir::FrozenRewritePatternSet frozen{std::move(patterns)};
    if (mlir::failed(mlir::applyFullConversion(mod.module_op.getOperation(), target, frozen))) {
        std::println("Failed to lower to LLVM dialect");
        return false;
    }
    return true;
}
