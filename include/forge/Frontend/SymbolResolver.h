#pragma once
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/AST.h"
#include <string>
#include <unordered_set>

namespace forge {

class SymbolResolver {
  public:
    SymbolResolver(DiagnosticEmitter &emitter) : emitter(emitter) {}

    bool resolve(const ast::Module &module);

#define __AST_NODE_TYPE(TYPE) void check(const ast::TYPE &);
    AST_NODE_TYPES
#undef __AST_NODE_TYPE

  private:
    DiagnosticEmitter &emitter;
    std::unordered_set<std::string> symbols;
};

} // namespace forge
