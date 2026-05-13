#include "forge/Conversion/ASTToMLIR/MLIRGenerator.h"
#include "forge/Frontend/AST.h"
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

mlir::Value MLIRGenerator::generate(const ast::Identifier &id) { return value_map.at(id.name); }

mlir::Value MLIRGenerator::generate(const ast::LetBinding &binding) {
    value_map[binding.name] = binding.value->visit(*this);
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

    for (const auto &expression : module_node.nodes) {
        expression->visit(*this);
    }

    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc());
    return module_op;
}

mlir::Value MLIRGenerator::generate(const ast::BinaryOperator &op) {
    auto lhs = op.lhs->visit(*this);
    auto rhs = op.rhs->visit(*this);
    switch (op.op) {
    case ast::BinaryOperator::OpType::Add:
        return mlir::arith::AddIOp::create(builder, builder.getUnknownLoc(), lhs, rhs);
    case ast::BinaryOperator::OpType::Sub:
        return mlir::arith::SubIOp::create(builder, builder.getUnknownLoc(), lhs, rhs);
    case ast::BinaryOperator::OpType::Mul:
        return mlir::arith::MulIOp::create(builder, builder.getUnknownLoc(), lhs, rhs);
    case ast::BinaryOperator::OpType::Div:
        return mlir::arith::DivSIOp::create(builder, builder.getUnknownLoc(), lhs, rhs);
    }
}

mlir::Value MLIRGenerator::generate(const ast::ReturnStmt &return_stmt) {
    auto return_value = return_stmt.expr->visit(*this);
    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc(), {return_value});
    return nullptr;
}

mlir::Type MLIRGenerator::resolve_type(const ast::UnresolvedTypeRef &type_ref) {
    if (type_ref.name == "i32") {
        return mlir::IntegerType::get(&ctx, 32);
    }
    if (type_ref.name == "i64") {
        return mlir::IntegerType::get(&ctx, 64);
    }
    if (type_ref.name == "f32") {
        return mlir::Float32Type::get(&ctx);
    }
    if (type_ref.name == "f64") {
        return mlir::Float64Type::get(&ctx);
    }
    return {};
}

mlir::Value MLIRGenerator::generate(const ast::FunctionDecl &function_declaration) {
    std::vector<mlir::Type> param_types;
    param_types.reserve(function_declaration.params.size());
    for (const auto &[_, type_ref] : function_declaration.params) {
        param_types.push_back(resolve_type(type_ref));
    }

    auto return_type = resolve_type(function_declaration.return_type);
    llvm::SmallVector<mlir::Type, 1> return_types;
    if (return_type) {
        return_types.push_back(return_type);
    }

    auto function_type = mlir::FunctionType::get(&ctx, param_types, return_types);

    auto saved_ip = builder.saveInsertionPoint();
    auto module_op = saved_ip.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();
    builder.setInsertionPointToEnd(&module_op.getBodyRegion().front());

    auto func = mlir::func::FuncOp::create(builder, builder.getUnknownLoc(),
                                           function_declaration.name, function_type);
    auto *entry = func.addEntryBlock();
    builder.setInsertionPointToStart(entry);

    auto saved_value_map = std::move(value_map);
    value_map.clear();
    for (size_t i = 0; i < function_declaration.params.size(); ++i) {
        value_map[function_declaration.params[i].first] = entry->getArgument(i);
    }

    mlir::Value last_value{};
    for (const auto &stmt : function_declaration.nodes) {
        last_value = stmt->visit(*this);
    }

    if (entry->empty() || !entry->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        if (last_value) {
            mlir::func::ReturnOp::create(builder, builder.getUnknownLoc(), last_value);
        } else {
            mlir::func::ReturnOp::create(builder, builder.getUnknownLoc());
        }
    }

    value_map = std::move(saved_value_map);
    builder.restoreInsertionPoint(saved_ip);
    return nullptr;
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
