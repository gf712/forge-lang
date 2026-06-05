#include "forge/Frontend/SymbolResolver.h"
#include "forge/Frontend/AST.h"
#include <cassert>

// Visitor dispatch: routes each AST node's resolve() call to the correct check() overload.
#define __AST_NODE_TYPE(X)                                                                         \
    void ast::X::resolve(forge::SymbolResolver &sr) const { sr.check(*this); }
AST_NODE_TYPES
#undef __AST_NODE_TYPE

namespace forge {

bool SymbolResolver::resolve(const ast::Module &module) {
    // Pre-register every top-level function so calls resolve regardless of source
    // order (forward references) and recursive calls see their own name.
    for (const auto &node : module.nodes) {
        if (const auto *fn = dynamic_cast<const ast::FunctionDecl *>(node.get())) {
            functions.insert(fn->name);
        }
    }
    for (const auto &node : module.nodes) {
        node->resolve(*this);
    }
    return !emitter.has_errors();
}

void SymbolResolver::check(const ast::IntLiteral &) {}

void SymbolResolver::check(const ast::BinaryOperator &op) {
    op.lhs->resolve(*this);
    op.rhs->resolve(*this);
}

void SymbolResolver::check(const ast::Identifier &id) {
    if (!variables.contains(id.name)) {
        emitter.error(id.loc, "use of undefined variable '" + id.name + "'", "not defined");
    }
}

void SymbolResolver::check(const ast::Call &call) {
    // A direct call by name resolves against the function namespace; anything else
    // (e.g. calling the result of an expression) resolves as an ordinary value.
    if (const auto *id = dynamic_cast<const ast::Identifier *>(call.callee.get())) {
        if (!functions.contains(id->name)) {
            emitter.error(id->loc, "call to undefined function '" + id->name + "'",
                          "not a function");
        }
    } else {
        call.callee->resolve(*this);
    }
    for (const auto &arg : call.args) {
        arg->resolve(*this);
    }
}

void SymbolResolver::check(const ast::IntrinsicCall &intrinsic) {
    // The intrinsic name is a compiler builtin, not a user symbol; just resolve args.
    for (const auto &arg : intrinsic.args) {
        arg->resolve(*this);
    }
}

void SymbolResolver::check(const ast::LetBinding &binding) {
    // Resolve the value expression before adding the name so `let x = x` is an error.
    binding.value->resolve(*this);
    variables.insert(binding.name);
}

void SymbolResolver::check(const ast::ReturnStmt &return_stmt) { return_stmt.expr->resolve(*this); }

void SymbolResolver::check(const ast::FunctionDecl &function_decl) {
    // Each body gets a fresh variable scope seeded with its parameters; functions
    // stay visible because they live in their own namespace.
    auto saved = std::move(variables);
    variables.clear();
    for (const auto &[param_name, _] : function_decl.params) {
        variables.insert(param_name);
    }
    for (const auto &stmt : function_decl.nodes) {
        stmt->resolve(*this);
    }
    variables = std::move(saved);
}

void SymbolResolver::check(const ast::TestDecl &test_decl) {
    auto saved = std::move(variables);
    variables.clear();
    for (const auto &stmt : test_decl.nodes) {
        stmt->resolve(*this);
    }
    variables = std::move(saved);
}

void SymbolResolver::check(const ast::Module &) {
    assert(false && "ast::Module is not a resolvable expression");
}

} // namespace forge
