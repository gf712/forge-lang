#pragma once
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/AST.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Types.h"

namespace forge {

class TypeChecker {
  public:
    TypeChecker(mlir::MLIRContext &ctx, DiagnosticEmitter &emitter) : ctx(ctx), emitter(emitter) {}

    bool check(const ast::Module &module);

    mlir::Type infer(const ast::IntLiteral &);
    mlir::Type infer(const ast::BinaryOperator &);
    mlir::Type infer(const ast::Module &);

  private:
    mlir::MLIRContext &ctx;
    DiagnosticEmitter &emitter;
};

} // namespace forge
