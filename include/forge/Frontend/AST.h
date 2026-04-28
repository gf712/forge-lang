#pragma once
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "llvm/Support/SourceMgr.h"
#include <memory>
#include <vector>

struct MLIRGenerator;
namespace forge {
class TypeChecker;
}

namespace ast {

#define AST_NODE_TYPES                                                                             \
    __AST_NODE_TYPE(BinaryOperator)                                                                \
    __AST_NODE_TYPE(IntLiteral)                                                                    \
    __AST_NODE_TYPE(Module)

enum class ASTNodeType {
#define __AST_NODE_TYPE(X) X,
    AST_NODE_TYPES
#undef __AST_NODE_TYPE
};

struct Node {
    llvm::SMRange loc;
    virtual ~Node() = default;
    virtual mlir::Value visit(MLIRGenerator &) const = 0;
    virtual mlir::Type type_of(forge::TypeChecker &) const = 0;
};

struct Expression : Node {};

struct Literal : Expression {};

struct IntLiteral final : Literal {
    int value;
    IntLiteral(int value) : value(value) {}
    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
};

struct Operator : Expression {};

struct BinaryOperator final : Operator {
    enum class OpType { Add };

    OpType op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;

    BinaryOperator(OpType op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
};

struct Module final : Node {
    std::vector<std::unique_ptr<Expression>> nodes;

    Module(std::vector<std::unique_ptr<Expression>> nodes) : nodes(std::move(nodes)) {}

    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
};

} // namespace ast
