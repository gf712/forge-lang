#pragma once
#include "forge/Frontend/AST.h"
#include "forge/IR/Module.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"

struct MLIRGenerator {
    mlir::OpBuilder builder;
    mlir::MLIRContext &ctx;

    mlir::Value generate(const ast::IntLiteral &value);
    mlir::Value generate(const ast::Module &module_node);
    mlir::ModuleOp generate_module(const ast::Module &module_node);
    mlir::Value generate(const ast::BinaryOperator &op);

    static forge::CompiledModule compile(std::unique_ptr<ast::Module> module_node);
};
