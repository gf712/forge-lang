#include "forge/Conversion/ASTToMLIR/MLIRGenerator.h"
#include "forge/Frontend/AST.h"
#include "forge/Frontend/TypeRegistry.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
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

mlir::Value MLIRGenerator::generate(const ast::Call &call) {
    const auto *id = dynamic_cast<const ast::Identifier *>(call.callee.get());
    assert(id && "only direct calls by name are supported");

    llvm::SmallVector<mlir::Value> args;
    args.reserve(call.args.size());
    for (const auto &arg : call.args) {
        args.push_back(arg->visit(*this));
    }

    auto module_op = builder.getInsertionBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();
    auto callee = module_op.lookupSymbol<mlir::func::FuncOp>(id->name);
    assert(callee && "callee should have been declared; checked by SymbolResolver");

    auto call_op = mlir::func::CallOp::create(builder, builder.getUnknownLoc(), callee, args);
    return call_op.getNumResults() ? call_op.getResult(0) : mlir::Value{};
}

mlir::Value MLIRGenerator::generate(const ast::LetBinding &binding) {
    value_map[binding.name] = binding.value->visit(*this);
    return {};
}

mlir::ModuleOp MLIRGenerator::generate_module(const ast::Module &module_node, bool run_tests) {
    auto module_op = mlir::ModuleOp::create(builder.getUnknownLoc());
    builder.setInsertionPointToStart(&module_op.getBodyRegion().front());

    auto function_type = mlir::FunctionType::get(&ctx, {}, {});
    mlir::NamedAttribute attrs{"llvm.emit_c_interface", mlir::UnitAttr::get(&ctx)};
    auto func = mlir::func::FuncOp::create(builder, builder.getUnknownLoc(), "__main",
                                           function_type, {attrs});
    builder.setInsertionPointToStart(func.addEntryBlock());

    // Declare all function prototypes before generating any bodies so that calls
    // resolve regardless of definition order.
    for (const auto &node : module_node.nodes) {
        if (const auto *fn = dynamic_cast<const ast::FunctionDecl *>(node.get())) {
            declare_function(*fn, module_op);
        }
    }

    for (const auto &expression : module_node.nodes) {
        expression->visit(*this);
    }

    if (run_tests) {
        for (const auto &sym : test_symbols) {
            auto callee = module_op.lookupSymbol<mlir::func::FuncOp>(sym);
            assert(callee && "test function should have been emitted");
            mlir::func::CallOp::create(builder, builder.getUnknownLoc(), callee,
                                       mlir::ValueRange{});
        }
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
    case ast::BinaryOperator::OpType::Eq:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::eq, lhs, rhs);
    case ast::BinaryOperator::OpType::Ne:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::ne, lhs, rhs);
    case ast::BinaryOperator::OpType::Lt:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::slt, lhs, rhs);
    case ast::BinaryOperator::OpType::Le:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::sle, lhs, rhs);
    case ast::BinaryOperator::OpType::Gt:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::sgt, lhs, rhs);
    case ast::BinaryOperator::OpType::Ge:
        return mlir::arith::CmpIOp::create(builder, builder.getUnknownLoc(),
                                           mlir::arith::CmpIPredicate::sge, lhs, rhs);
    }
    return {};
}

mlir::Value MLIRGenerator::generate(const ast::IntrinsicCall &intrinsic) {
    assert(intrinsic.name == "assert" && "unknown intrinsic should be rejected by TypeChecker");
    auto cond = intrinsic.args[0]->visit(*this);

    const char *start = intrinsic.args[0]->loc.Start.getPointer();
    const char *end = intrinsic.args[0]->loc.End.getPointer();
    std::string msg = "assertion failed: " + std::string(start, end - start);

    mlir::cf::AssertOp::create(builder, builder.getUnknownLoc(), cond, msg);
    return {};
}

mlir::Value MLIRGenerator::generate(const ast::ReturnStmt &return_stmt) {
    auto return_value = return_stmt.expr->visit(*this);
    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc(), {return_value});
    return nullptr;
}

mlir::Type MLIRGenerator::resolve_type(const ast::UnresolvedTypeRef &type_ref) {
    auto it = builtin_types.types().find(type_ref.name);
    if (it != builtin_types.types().end()) {
        return it->second;
    }
    return {};
}

mlir::func::FuncOp MLIRGenerator::declare_function(const ast::FunctionDecl &function_declaration,
                                                   mlir::ModuleOp module_op) {
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
    builder.setInsertionPointToEnd(&module_op.getBodyRegion().front());
    auto func = mlir::func::FuncOp::create(builder, builder.getUnknownLoc(),
                                           function_declaration.name, function_type);
    builder.restoreInsertionPoint(saved_ip);
    return func;
}

mlir::Value MLIRGenerator::generate(const ast::FunctionDecl &function_declaration) {
    auto saved_ip = builder.saveInsertionPoint();
    auto module_op = saved_ip.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();

    auto func = module_op.lookupSymbol<mlir::func::FuncOp>(function_declaration.name);
    assert(func && "function prototype should have been declared");
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

mlir::Value MLIRGenerator::generate(const ast::TestDecl &function_declaration) {
    auto function_type = mlir::FunctionType::get(&ctx, {}, {});

    auto saved_ip = builder.saveInsertionPoint();
    auto module_op = saved_ip.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();
    builder.setInsertionPointToEnd(&module_op.getBodyRegion().front());

    // Tests get an anonymous, unique symbol so never becomes a referenceable symbol.
    std::string sym = "__test." + std::to_string(test_symbols.size());
    auto func = mlir::func::FuncOp::create(builder, builder.getUnknownLoc(), sym, function_type);
    func.setPrivate();
    func->setAttr("forge.test_name", mlir::StringAttr::get(&ctx, function_declaration.name));
    test_symbols.push_back(sym);
    auto *entry = func.addEntryBlock();
    builder.setInsertionPointToStart(entry);

    auto saved_value_map = std::move(value_map);
    value_map.clear();

    mlir::Value last_value{};
    for (const auto &stmt : function_declaration.nodes) {
        last_value = stmt->visit(*this);
    }

    if (entry->empty() || !entry->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        mlir::func::ReturnOp::create(builder, builder.getUnknownLoc());
    }

    value_map = std::move(saved_value_map);
    builder.restoreInsertionPoint(saved_ip);
    return nullptr;
}

forge::CompiledModule MLIRGenerator::compile(std::unique_ptr<ast::Module> module_node,
                                             bool run_tests) {
    forge::CompiledModule mod{.ctx = std::make_unique<mlir::MLIRContext>()};
    mod.ctx->loadDialect<mlir::arith::ArithDialect>();
    mod.ctx->loadDialect<mlir::cf::ControlFlowDialect>();
    mod.ctx->loadDialect<mlir::func::FuncDialect>();

    MLIRGenerator generator{
        .builder = mlir::OpBuilder{mod.ctx.get()},
        .ctx = *mod.ctx,
        .builtin_types = forge::BuiltinTypeRegistry{*mod.ctx},
    };
    mod.module_op = generator.generate_module(*module_node, run_tests);
    return mod;
}
