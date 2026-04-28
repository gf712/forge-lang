#include "forge/Frontend/TypeChecker.h"
#include "mlir/IR/BuiltinTypes.h"

// Visitor dispatch: routes each AST node's type_of() call to the correct infer() overload.
#define __AST_NODE_TYPE(X)                                                                         \
    mlir::Type ast::X::type_of(forge::TypeChecker &tc) const { return tc.infer(*this); }
AST_NODE_TYPES
#undef __AST_NODE_TYPE

namespace forge {

bool TypeChecker::check(const ast::Module &module) {
    for (const auto &expr : module.nodes)
        expr->type_of(*this);
    return !emitter.has_errors();
}

mlir::Type TypeChecker::infer(const ast::IntLiteral &) { return mlir::IntegerType::get(&ctx, 64); }

mlir::Type TypeChecker::infer(const ast::BinaryOperator &op) {
    auto lhs = op.lhs->type_of(*this);
    auto rhs = op.rhs->type_of(*this);

    switch (op.op) {
    case ast::BinaryOperator::OpType::Add: {
        if (!lhs || !rhs)
            return {};
        if (!llvm::isa<mlir::IntegerType>(lhs) || !llvm::isa<mlir::IntegerType>(rhs)) {
            emitter.error(op.loc, "'+' requires integer operands", "expected `int`");
            return {};
        }
        return lhs;
    }
    }
}

mlir::Type TypeChecker::infer(const ast::Module &) {
    assert(false && "ast::Module is not a typed expression");
    return {};
}

} // namespace forge
