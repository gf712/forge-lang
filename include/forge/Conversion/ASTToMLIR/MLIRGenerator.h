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

    mlir::Value generate(const ast::IntLiteral &value);
    mlir::Value generate(const ast::Module &module_node);
    mlir::Value generate(const ast::Identifier &id);
    mlir::Value generate(const ast::LetBinding &binding);
    mlir::ModuleOp generate_module(const ast::Module &module_node);
    mlir::Value generate(const ast::BinaryOperator &op);

    static forge::CompiledModule compile(std::unique_ptr<ast::Module> module_node);
};
