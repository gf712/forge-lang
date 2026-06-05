#pragma once
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/AST.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/StringMap.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace forge {

class TypeChecker {
  public:
    TypeChecker(mlir::MLIRContext &ctx, DiagnosticEmitter &emitter);

    bool check(const ast::Module &module);

#define __AST_NODE_TYPE(TYPE) mlir::Type infer(const ast::TYPE &);
    AST_NODE_TYPES
#undef __AST_NODE_TYPE

    mlir::Type resolve(const ast::UnresolvedTypeRef &);

  private:
    struct FunctionSignature {
        std::vector<mlir::Type> params;
        mlir::Type return_type;
    };

    mlir::MLIRContext &ctx;
    DiagnosticEmitter &emitter;
    llvm::StringMap<mlir::Type> type_registry; // built-ins pre-populated; user types added later
    std::unordered_map<std::string, mlir::Type> symbol_types;
    std::unordered_map<std::string, FunctionSignature> function_signatures;
};

} // namespace forge
