#pragma once
#include "forge/Frontend/AST.h"
#include "forge/Frontend/TypeRegistry.h"
#include "forge/IR/Module.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include <string>
#include <unordered_map>
#include <vector>

struct MLIRGenerator {
    mlir::OpBuilder builder;
    mlir::MLIRContext &ctx;
    std::unordered_map<std::string, mlir::Value> value_map;
    std::vector<std::string> test_symbols; // anonymous symbols of emitted test funcs, in order
    forge::BuiltinTypeRegistry builtin_types;

#define __AST_NODE_TYPE(TYPE) mlir::Value generate(const ast::TYPE &);
    AST_NODE_TYPES
#undef __AST_NODE_TYPE

    mlir::Type resolve_type(const ast::UnresolvedTypeRef &type_ref);
    mlir::func::FuncOp declare_function(const ast::FunctionDecl &function_declaration,
                                        mlir::ModuleOp module_op);
    mlir::ModuleOp generate_module(const ast::Module &module_node, bool run_tests);

    static forge::CompiledModule compile(std::unique_ptr<ast::Module> module_node, bool run_tests);
};
