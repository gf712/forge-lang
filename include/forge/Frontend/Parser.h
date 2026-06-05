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
# Top-level. `test` is module-scope only; `fn` can also appear inside a body.
module          <- module_item*
module_item     <- test_definition / stmt

fn_definition   <- 'fn' ident '(' params? ')' '->' ident block
test_definition <- 'test' string_literal block

block           <- '{' stmt* expression? '}'
params          <- param (',' param)*
param           <- ident ':' ident

# Statements
stmt            <- fn_definition / let_binding / return_stmt / expression_stmt
let_binding     <- 'let' ident (':' ident)? '=' expression ';'
return_stmt     <- 'return' expression ';'
expression_stmt <- expression ';'

# Expressions — precedence climb, low to high.
# postfix hosts call/index/field suffixes; currently only call_suffix is
# defined and has no AST node yet, so calls silently parse as their callee.
expression      <- comparison
comparison      <- additive (cmp_op additive)?
additive        <- multiplicative (add_op multiplicative)*
multiplicative  <- postfix (mul_op postfix)*
postfix         <- primary call_suffix*
call_suffix     <- '(' arg_list? ')'
arg_list        <- expression (',' expression)*
intrinsic_call  <- '@' ident '(' arg_list? ')'
primary         <- intrinsic_call / '(' expression ')' / integer / ident

add_op          <- < '+' / '-' >
mul_op          <- < '*' / '/' >
cmp_op          <- < '==' / '!=' / '<=' / '>=' / '<' / '>' >

# Lexical
keyword         <- ('fn' / 'test' / 'let' / 'return') ![a-zA-Z0-9_]
ident           <- !keyword < [a-zA-Z_][a-zA-Z0-9_]* >
integer         <- < [0-9]+ >
string_literal  <- '"' < (!'"' !'\n' .)* > '"'
%whitespace     <- ([ \t\r\n] / '//' (!'\n' .)* '\n'?)*
)";

    static std::unique_ptr<ast::Module> parse(std::filesystem::path path,
                                              llvm::SourceMgr &source_mgr,
                                              forge::DiagnosticEmitter &emitter);
};
