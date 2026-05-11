#include "forge/Frontend/TypeChecker.h"
#include "mlir/IR/BuiltinTypes.h"

// Visitor dispatch: routes each AST node's type_of() call to the correct infer() overload.
#define __AST_NODE_TYPE(X)                                                                         \
    mlir::Type ast::X::type_of(forge::TypeChecker &tc) const { return tc.infer(*this); }
AST_NODE_TYPES
#undef __AST_NODE_TYPE

// TypeExpr dispatch: UnresolvedTypeRef delegates resolution to TypeChecker.
mlir::Type ast::UnresolvedTypeRef::resolve(forge::TypeChecker &tc) const {
    return tc.resolve(*this);
}

namespace forge {

TypeChecker::TypeChecker(mlir::MLIRContext &ctx, DiagnosticEmitter &emitter)
    : ctx(ctx), emitter(emitter) {
    type_registry["i32"] = mlir::IntegerType::get(&ctx, 32);
    type_registry["i64"] = mlir::IntegerType::get(&ctx, 64);
    type_registry["f32"] = mlir::Float32Type::get(&ctx);
    type_registry["f64"] = mlir::Float64Type::get(&ctx);
}

bool TypeChecker::check(const ast::Module &module) {
    for (const auto &node : module.nodes)
        node->type_of(*this);
    return !emitter.has_errors();
}

mlir::Type TypeChecker::resolve(const ast::UnresolvedTypeRef &type_ref) {
    auto it = type_registry.find(type_ref.name);
    if (it == type_registry.end()) {
        emitter.error(type_ref.loc, "unknown type '" + type_ref.name + "'", "unknown type");
        return {};
    }
    return it->second;
}

mlir::Type TypeChecker::infer(const ast::IntLiteral &) { return mlir::IntegerType::get(&ctx, 64); }

mlir::Type TypeChecker::infer(const ast::BinaryOperator &op) {
    auto lhs = op.lhs->type_of(*this);
    auto rhs = op.rhs->type_of(*this);

    if (!lhs || !rhs)
        return {};

    auto check_int = [&](const char *sym) -> mlir::Type {
        if (!llvm::isa<mlir::IntegerType>(lhs) || !llvm::isa<mlir::IntegerType>(rhs)) {
            emitter.error(op.loc, std::string("'") + sym + "' requires integer operands",
                          "expected integer");
            return {};
        }
        return lhs;
    };

    switch (op.op) {
    case ast::BinaryOperator::OpType::Add:
        return check_int("+");
    case ast::BinaryOperator::OpType::Sub:
        return check_int("-");
    case ast::BinaryOperator::OpType::Mul:
        return check_int("*");
    case ast::BinaryOperator::OpType::Div:
        return check_int("/");
    }
}

mlir::Type TypeChecker::infer(const ast::Identifier &id) {
    auto it = symbol_types.find(id.name);
    if (it == symbol_types.end())
        return {};
    return it->second;
}

mlir::Type TypeChecker::infer(const ast::LetBinding &binding) {
    mlir::Type inferred = binding.value->type_of(*this);

    if (binding.type_annotation) {
        mlir::Type annotated = binding.type_annotation->resolve(*this);
        if (annotated && inferred && annotated != inferred) {
            emitter.error(binding.type_annotation->loc,
                          "type annotation '" + std::string(binding.type_annotation->type_name()) +
                              "' does not match value type",
                          "annotated here");
            return {};
        }
        if (annotated) {
            symbol_types[binding.name] = annotated;
            return annotated;
        }
    }

    if (inferred)
        symbol_types[binding.name] = inferred;
    return inferred;
}

mlir::Type TypeChecker::infer(const ast::Module &) {
    assert(false && "ast::Module is not a typed expression");
    return {};
}

} // namespace forge
