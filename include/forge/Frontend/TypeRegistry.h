#pragma once

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Types.h"
#include <llvm/ADT/StringMap.h>

namespace forge {
class BuiltinTypeRegistry {
    llvm::StringMap<mlir::Type> type_registry;

  public:
    BuiltinTypeRegistry(mlir::MLIRContext &ctx) {
        type_registry["bool"] = mlir::IntegerType::get(&ctx, 1);
        type_registry["i8"] = mlir::IntegerType::get(&ctx, 8);
        type_registry["i16"] = mlir::IntegerType::get(&ctx, 16);
        type_registry["i32"] = mlir::IntegerType::get(&ctx, 32);
        type_registry["i64"] = mlir::IntegerType::get(&ctx, 64);
        type_registry["u8"] = mlir::IntegerType::get(&ctx, 8, mlir::IntegerType::Signless);
        type_registry["u16"] = mlir::IntegerType::get(&ctx, 16, mlir::IntegerType::Signless);
        type_registry["u32"] = mlir::IntegerType::get(&ctx, 32, mlir::IntegerType::Signless);
        type_registry["u64"] = mlir::IntegerType::get(&ctx, 64, mlir::IntegerType::Signless);
        type_registry["bf16"] = mlir::BFloat16Type::get(&ctx);
        type_registry["f16"] = mlir::Float16Type::get(&ctx);
        type_registry["f32"] = mlir::Float32Type::get(&ctx);
        type_registry["f64"] = mlir::Float64Type::get(&ctx);
    }

    const llvm::StringMap<mlir::Type> &types() const { return type_registry; }
};
} // namespace forge
