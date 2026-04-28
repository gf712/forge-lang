#pragma once
#include "forge/Frontend/AST.h"
#include "llvm/Support/SourceMgr.h"
#include <filesystem>
#include <memory>
#include <string_view>

struct Parser {
    // PEG grammar for Forge — kept constexpr so a future consteval make_parser()
    // can replace the runtime peg::parser with zero changes to the grammar itself.
    static constexpr std::string_view k_grammar = R"(
module    <- expr*
expr      <- additive
additive  <- atom ('+' atom)*
atom      <- integer
integer   <- < [0-9]+ >
%whitespace <- ([ \t\r\n] / '//' (!'\n' .)* '\n'?)*
)";

    static std::unique_ptr<ast::Module> parse(std::filesystem::path path,
                                              llvm::SourceMgr &source_mgr);
};
