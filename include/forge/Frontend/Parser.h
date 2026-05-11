#pragma once
#include "forge/Diagnostics/Emitter.h"
#include "forge/Frontend/AST.h"
#include "llvm/Support/SourceMgr.h"
#include <filesystem>
#include <memory>
#include <string_view>

struct Parser {
    // PEG grammar for Forge — kept constexpr so a future consteval make_parser()
    // can replace the runtime peg::parser with zero changes to the grammar itself.
    static constexpr std::string_view kGrammar = R"(
module         <- stmt*
stmt           <- fn_definition / let_binding / return_stmt / additive
fn_definition  <- 'fn' ident '(' params? ')' '->' ident '{' stmt* '}'
params         <- param (',' param)*
param          <- ident ':' ident
let_binding    <- 'let' ident (':' ident)? '=' additive ';'
return_stmt    <- 'return' additive ';'
additive       <- multiplicative (add_op multiplicative)*
multiplicative <- atom (mul_op atom)*
add_op         <- < '+' / '-' >
mul_op         <- < '*' / '/' >
atom           <- '(' additive ')' / integer / ident
keyword        <- ('fn' / 'let' / 'return') ![a-zA-Z0-9_]
ident          <- !keyword < [a-zA-Z_][a-zA-Z0-9_]* >
integer        <- < [0-9]+ >
%whitespace    <- ([ \t\r\n] / '//' (!'\n' .)* '\n'?)*
)";

    static std::unique_ptr<ast::Module> parse(std::filesystem::path path,
                                              llvm::SourceMgr &source_mgr,
                                              forge::DiagnosticEmitter &emitter);
};
