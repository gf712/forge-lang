#pragma once
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/AST.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/StringMap.h"
#include <string>
#include <unordered_map>

namespace forge {

class TypeChecker {
  public:
    TypeChecker(mlir::MLIRContext &ctx, DiagnosticEmitter &emitter);

    bool check(const ast::Module &module);

    mlir::Type infer(const ast::IntLiteral &);
    mlir::Type infer(const ast::BinaryOperator &);
    mlir::Type infer(const ast::Identifier &);
    mlir::Type infer(const ast::LetBinding &);
    mlir::Type infer(const ast::Module &);

    mlir::Type resolve(const ast::UnresolvedTypeRef &);

  private:
    mlir::MLIRContext &ctx;
    DiagnosticEmitter &emitter;
    llvm::StringMap<mlir::Type> type_registry; // built-ins pre-populated; user types added later
    std::unordered_map<std::string, mlir::Type> symbol_types;
};

} // namespace forge
