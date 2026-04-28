#include "forge/Conversion/ASTToMLIR/MLIRGenerator.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"

// Visitor dispatch: routes each AST node's visit() call to the correct generate() overload.
#define __AST_NODE_TYPE(X)                                                                         \
    mlir::Value ast::X::visit(MLIRGenerator &generator) const { return generator.generate(*this); }
AST_NODE_TYPES
#undef __AST_NODE_TYPE

mlir::Value MLIRGenerator::generate(const ast::IntLiteral &value) {
    return mlir::arith::ConstantIntOp::create(builder, builder.getUnknownLoc(), value.value, 64);
}

mlir::Value MLIRGenerator::generate(const ast::Module &) {
    assert(false && "ast::Module must be lowered via generate_module()");
    return {};
}

mlir::ModuleOp MLIRGenerator::generate_module(const ast::Module &module_node) {
    auto module_op = mlir::ModuleOp::create(builder.getUnknownLoc());
    builder.setInsertionPointToStart(&module_op.getBodyRegion().front());

    auto function_type = mlir::FunctionType::get(&ctx, {}, {});
    mlir::NamedAttribute attrs{"llvm.emit_c_interface", mlir::UnitAttr::get(&ctx)};
    auto func = mlir::func::FuncOp::create(builder, builder.getUnknownLoc(), "__main",
                                           function_type, {attrs});
    builder.setInsertionPointToStart(func.addEntryBlock());

    for (const auto &expression : module_node.nodes)
        expression->visit(*this);

    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc());
    return module_op;
}

mlir::Value MLIRGenerator::generate(const ast::BinaryOperator &op) {
    auto lhs = op.lhs->visit(*this);
    auto rhs = op.rhs->visit(*this);
    switch (op.op) {
    case ast::BinaryOperator::OpType::Add:
        return mlir::arith::AddIOp::create(builder, builder.getUnknownLoc(), lhs, rhs);
    }
}

forge::CompiledModule MLIRGenerator::compile(std::unique_ptr<ast::Module> module_node) {
    forge::CompiledModule mod{.ctx = std::make_unique<mlir::MLIRContext>()};
    mod.ctx->loadDialect<mlir::arith::ArithDialect>();
    mod.ctx->loadDialect<mlir::func::FuncDialect>();

    MLIRGenerator generator{
        .builder = mlir::OpBuilder{mod.ctx.get()},
        .ctx = *mod.ctx,
    };
    mod.module_op = generator.generate_module(*module_node);
    return mod;
}
