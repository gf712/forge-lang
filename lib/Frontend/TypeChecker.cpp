#include "forge/Frontend/TypeChecker.h"
#include "forge/Frontend/TypeRegistry.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"

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
    BuiltinTypeRegistry builtin_types{ctx};
    for (const auto &[name, type] : builtin_types.types()) {
        type_registry.try_emplace(name, type);
    }
}

bool TypeChecker::check(const ast::Module &module) {
    // Pre-register signatures so calls type-check against any function regardless
    // of source order (forward references) and recursion.
    for (const auto &node : module.nodes) {
        if (const auto *fn = dynamic_cast<const ast::FunctionDecl *>(node.get())) {
            FunctionSignature sig;
            sig.params.reserve(fn->params.size());
            for (const auto &[_, param_type] : fn->params) {
                sig.params.push_back(param_type.resolve(*this));
            }
            sig.return_type = fn->return_type.resolve(*this);
            functions[fn] = sig;
            function_signatures[fn->name] = std::move(sig);
        }
    }
    for (const auto &node : module.nodes) {
        node->type_of(*this);
    }
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

    if (!lhs || !rhs) {
        return {};
    }

    auto check_int = [&](const char *sym) -> mlir::Type {
        if (!llvm::isa<mlir::IntegerType>(lhs) || !llvm::isa<mlir::IntegerType>(rhs)) {
            emitter.error(op.loc, std::string("'") + sym + "' requires integer operands",
                          "expected integer");
            return {};
        }
        return lhs;
    };

    // Comparisons require integer operands of the same type and yield a bool (i1).
    auto check_cmp = [&](const char *sym) -> mlir::Type {
        if (!llvm::isa<mlir::IntegerType>(lhs) || !llvm::isa<mlir::IntegerType>(rhs)) {
            emitter.error(op.loc, std::string("'") + sym + "' requires integer operands",
                          "expected integer");
            return {};
        }
        if (lhs != rhs) {
            emitter.error(op.loc, std::string("'") + sym + "' requires operands of the same type",
                          "mismatched types");
            return {};
        }
        return mlir::IntegerType::get(&ctx, 1);
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
    case ast::BinaryOperator::OpType::Eq:
        return check_cmp("==");
    case ast::BinaryOperator::OpType::Ne:
        return check_cmp("!=");
    case ast::BinaryOperator::OpType::Lt:
        return check_cmp("<");
    case ast::BinaryOperator::OpType::Le:
        return check_cmp("<=");
    case ast::BinaryOperator::OpType::Gt:
        return check_cmp(">");
    case ast::BinaryOperator::OpType::Ge:
        return check_cmp(">=");
    }
    return {};
}

mlir::Type TypeChecker::infer(const ast::Identifier &id) {
    auto it = symbol_types.find(id.name);
    if (it == symbol_types.end()) {
        return {};
    }
    return it->second;
}

mlir::Type TypeChecker::infer(const ast::Call &call) {
    const auto *id = dynamic_cast<const ast::Identifier *>(call.callee.get());
    if (!id) {
        emitter.error(call.loc, "callee is not a function", "not callable");
        return {};
    }

    auto it = function_signatures.find(id->name);
    if (it == function_signatures.end()) {
        // Undefined-function errors are already reported by the SymbolResolver.
        return {};
    }
    const auto &sig = it->second;

    if (call.args.size() != sig.params.size()) {
        emitter.error(call.loc,
                      "function '" + id->name + "' expects " + std::to_string(sig.params.size()) +
                          " argument(s) but got " + std::to_string(call.args.size()),
                      "wrong number of arguments");
        return sig.return_type;
    }

    for (size_t i = 0; i < call.args.size(); ++i) {
        auto arg_type = call.args[i]->type_of(*this);
        if (arg_type && sig.params[i] && arg_type != sig.params[i]) {
            emitter.error(call.args[i]->loc, "argument type does not match parameter type",
                          "mismatched type");
        }
    }
    return sig.return_type;
}

mlir::Type TypeChecker::infer(const ast::IntrinsicCall &intrinsic) {
    if (intrinsic.name == "assert") {
        if (intrinsic.args.size() != 1) {
            emitter.error(intrinsic.loc,
                          "@assert expects 1 argument but got " +
                              std::to_string(intrinsic.args.size()),
                          "wrong number of arguments");
            return {};
        }
        auto cond = intrinsic.args[0]->type_of(*this);
        auto i1 = mlir::IntegerType::get(&ctx, 1);
        if (cond && cond != i1) {
            emitter.error(intrinsic.args[0]->loc, "@assert condition must be a bool",
                          "expected bool");
        }
        // @assert is statement-position and produces no value.
        return {};
    }

    emitter.error(intrinsic.loc, "unknown intrinsic '@" + intrinsic.name + "'",
                  "unknown intrinsic");
    return {};
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

    if (inferred) {
        symbol_types[binding.name] = inferred;
    }
    return inferred;
}

mlir::Type TypeChecker::infer(const ast::ReturnStmt &return_stmt) {
    auto stmt_return_type = [this, &return_stmt]() -> mlir::Type {
        if (return_stmt.expr == nullptr) {
            return mlir::NoneType::get(&ctx);
        }
        return return_stmt.expr->type_of(*this);
    }();
    const auto &expected_return_type = return_type.top();
    if (stmt_return_type != expected_return_type) {
        // TODO stringify return type and show to user type mismatch
        emitter.error(return_stmt.loc, "return type mismatch", "type mismatch");
    }
    return {};
}

mlir::Type TypeChecker::infer(const ast::FunctionDecl &function_decl) {
    for (const auto &[param_name, param_type] : function_decl.params) {
        symbol_types[param_name] = param_type.resolve(*this);
    }
    return_type.push(functions.at(&function_decl).return_type);
    for (const auto &stmt : function_decl.nodes) {
        stmt->type_of(*this);
    }
    return_type.pop();
    return {};
}

mlir::Type TypeChecker::infer(const ast::TestDecl &test_decl) {
    return_type.push(mlir::NoneType::get(&ctx));
    for (const auto &stmt : test_decl.nodes) {
        stmt->type_of(*this);
    }
    return_type.pop();
    return {};
}

mlir::Type TypeChecker::infer(const ast::Module &) {
    assert(false && "ast::Module is not a typed expression");
    return {};
}

} // namespace forge
