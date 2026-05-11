#include "forge/Frontend/SymbolResolver.h"
#include <cassert>

// Visitor dispatch: routes each AST node's resolve() call to the correct check() overload.
#define __AST_NODE_TYPE(X)                                                                         \
    void ast::X::resolve(forge::SymbolResolver &sr) const { sr.check(*this); }
AST_NODE_TYPES
#undef __AST_NODE_TYPE

namespace forge {

bool SymbolResolver::resolve(const ast::Module &module) {
    for (const auto &node : module.nodes)
        node->resolve(*this);
    return !emitter.has_errors();
}

void SymbolResolver::check(const ast::IntLiteral &) {}

void SymbolResolver::check(const ast::BinaryOperator &op) {
    op.lhs->resolve(*this);
    op.rhs->resolve(*this);
}

void SymbolResolver::check(const ast::Identifier &id) {
    if (!symbols.contains(id.name))
        emitter.error(id.loc, "use of undefined variable '" + id.name + "'", "not defined");
}

void SymbolResolver::check(const ast::LetBinding &binding) {
    // Resolve the value expression before adding the name so `let x = x` is an error.
    binding.value->resolve(*this);
    symbols.insert(binding.name);
}

void SymbolResolver::check(const ast::Module &) {
    assert(false && "ast::Module is not a resolvable expression");
}

} // namespace forge
