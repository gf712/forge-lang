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

    void check(const ast::IntLiteral &);
    void check(const ast::BinaryOperator &);
    void check(const ast::Identifier &);
    void check(const ast::LetBinding &);
    void check(const ast::Module &);

  private:
    DiagnosticEmitter &emitter;
    std::unordered_set<std::string> symbols;
};

} // namespace forge
