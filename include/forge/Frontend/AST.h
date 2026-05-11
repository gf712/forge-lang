#pragma once
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "llvm/Support/SourceMgr.h"
#include <memory>
#include <string>
#include <vector>

struct MLIRGenerator;
namespace forge {
class TypeChecker;
class SymbolResolver;
} // namespace forge

namespace ast {

// TypeExpr hierarchy: consumed entirely by TypeChecker, never reaches MLIRGenerator.
struct TypeExpr {
    llvm::SMRange loc;
    virtual ~TypeExpr() = default;
    virtual mlir::Type resolve(forge::TypeChecker &) const = 0;
    virtual std::string_view type_name() const = 0;
};

struct UnresolvedTypeRef final : TypeExpr {
    std::string name;
    UnresolvedTypeRef(std::string name, llvm::SMRange loc) : name(std::move(name)) {
        this->loc = loc;
    }
    mlir::Type resolve(forge::TypeChecker &) const final;
    std::string_view type_name() const final { return name; }
};

#define AST_NODE_TYPES                                                                             \
    __AST_NODE_TYPE(BinaryOperator)                                                                \
    __AST_NODE_TYPE(Identifier)                                                                    \
    __AST_NODE_TYPE(IntLiteral)                                                                    \
    __AST_NODE_TYPE(LetBinding)                                                                    \
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
    virtual void resolve(forge::SymbolResolver &) const = 0;
};

struct Expression : Node {};

struct Literal : Expression {};

struct IntLiteral final : Literal {
    int value;
    IntLiteral(int value) : value(value) {}
    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
    void resolve(forge::SymbolResolver &) const final;
};

struct Operator : Expression {};

struct BinaryOperator final : Operator {
    enum class OpType { Add, Sub, Mul, Div };

    OpType op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;

    BinaryOperator(OpType op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
    void resolve(forge::SymbolResolver &) const final;
};

struct Identifier final : Expression {
    std::string name;
    Identifier(std::string name) : name(std::move(name)) {}
    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
    void resolve(forge::SymbolResolver &) const final;
};

struct LetBinding final : Node {
    std::string name;
    std::unique_ptr<TypeExpr> type_annotation; // null if no annotation written
    std::unique_ptr<Expression> value;
    LetBinding(std::string name, std::unique_ptr<TypeExpr> type_annotation,
               std::unique_ptr<Expression> value)
        : name(std::move(name)), type_annotation(std::move(type_annotation)),
          value(std::move(value)) {}
    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
    void resolve(forge::SymbolResolver &) const final;
};

struct Module final : Node {
    std::vector<std::unique_ptr<Node>> nodes;

    Module(std::vector<std::unique_ptr<Node>> nodes) : nodes(std::move(nodes)) {}

    mlir::Value visit(MLIRGenerator &) const final;
    mlir::Type type_of(forge::TypeChecker &) const final;
    void resolve(forge::SymbolResolver &) const final;
};

} // namespace ast
