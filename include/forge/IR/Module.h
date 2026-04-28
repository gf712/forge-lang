#pragma once
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include <memory>

namespace forge {

struct CompiledModule {
    std::unique_ptr<mlir::MLIRContext> ctx;
    mlir::ModuleOp module_op;
};

} // namespace forge
