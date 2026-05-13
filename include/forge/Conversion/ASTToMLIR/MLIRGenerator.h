#pragma once
#include "forge/Frontend/AST.h"
#include "forge/IR/Module.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include <string>
#include <unordered_map>

struct MLIRGenerator {
    mlir::OpBuilder builder;
    mlir::MLIRContext &ctx;
    std::unordered_map<std::string, mlir::Value> value_map;

#define __AST_NODE_TYPE(TYPE) mlir::Value generate(const ast::TYPE &);
    AST_NODE_TYPES
#undef __AST_NODE_TYPE

    mlir::Type resolve_type(const ast::UnresolvedTypeRef &type_ref);
    mlir::ModuleOp generate_module(const ast::Module &module_node);

    static forge::CompiledModule compile(std::unique_ptr<ast::Module> module_node);
};
