#include "forge/Frontend/Parser.h"
#include "forge/Frontend/AST.h"
#include "llvm/Support/MemoryBuffer.h"
#include <llvm/Support/SMLoc.h>
#include <peglib.h>

#include <print>
#include <ranges>

static llvm::SMRange to_range(const char *base, const peg::Ast &node) {
    auto s = llvm::SMLoc::getFromPointer(base + node.position);
    auto e = llvm::SMLoc::getFromPointer(base + node.position + node.length);
    return {s, e};
}

static std::unique_ptr<ast::Expression> convert(const peg::Ast &node, const char *base) {
    if (node.name == "integer") {
        auto lit = std::make_unique<ast::IntLiteral>(std::stoi(std::string(node.token)));
        lit->loc = to_range(base, node);
        return lit;
    }
    if (node.name == "ident") {
        auto id = std::make_unique<ast::Identifier>(std::string(node.token));
        id->loc = to_range(base, node);
        return id;
    }
    if ((node.name == "additive" || node.name == "multiplicative") && node.nodes.size() > 1) {
        // Children alternate: operand, op, operand, op, operand, ...
        auto result = convert(*node.nodes[0], base);
        for (size_t i = 1; i < node.nodes.size(); i += 2) {
            const auto &op_node = *node.nodes[i];
            auto rhs = convert(*node.nodes[i + 1], base);
            ast::BinaryOperator::OpType op_type;
            if (op_node.token == "+") {
                op_type = ast::BinaryOperator::OpType::Add;
            } else if (op_node.token == "-") {
                op_type = ast::BinaryOperator::OpType::Sub;
            } else if (op_node.token == "*") {
                op_type = ast::BinaryOperator::OpType::Mul;
            } else {
                op_type = ast::BinaryOperator::OpType::Div;
            }
            llvm::SMRange loc{result->loc.Start, rhs->loc.End};
            auto binop =
                std::make_unique<ast::BinaryOperator>(op_type, std::move(result), std::move(rhs));
            binop->loc = loc;
            result = std::move(binop);
        }
        return result;
    }
    // Pass-through: expr, atom, single-child additive
    if (!node.nodes.empty()) {
        return convert(*node.nodes[0], base);
    }
    return nullptr;
}

static std::unique_ptr<ast::Node> convert_stmt(const peg::Ast &node, const char *base) {
    if (node.name == "stmt") {
        return convert_stmt(*node.nodes[0], base);
    }
    if (node.name == "return_stmt") {
        auto expr = convert(*node.nodes[0], base);
        auto stmt = std::make_unique<ast::ReturnStmt>(std::move(expr));
        stmt->loc = to_range(base, node);
        return stmt;
    }
    if (node.name == "let_binding") {
        // Grammar: 'let' ident (':' ident)? '=' expr
        // Terminals are not captured, so children are: [ident, expr] or [ident, type_ident, expr]
        const auto &ident = *node.nodes.front();
        std::string name = std::string(ident.token);

        std::unique_ptr<ast::TypeExpr> type_annotation;
        const peg::Ast *expr_node = nullptr;

        if (node.nodes.size() == 3) {
            const auto &type_ast = *node.nodes[1];
            type_annotation = std::make_unique<ast::UnresolvedTypeRef>(std::string(type_ast.token),
                                                                       to_range(base, type_ast));
            expr_node = node.nodes[2].get();
        } else {
            expr_node = node.nodes[1].get();
        }

        auto value = convert(*expr_node, base);
        auto binding = std::make_unique<ast::LetBinding>(
            std::move(name), std::move(type_annotation), std::move(value));
        binding->loc = to_range(base, node);
        return binding;
    }
    if (node.name == "fn_definition") {
        std::string fn_name{node.nodes.front()->token};
        std::vector<ast::FunctionDecl::ParamType> params;
        if (auto parsed_params = node.nodes[1]; parsed_params->name == "params") {
            for (const auto &param : parsed_params->nodes) {
                assert(param->name == "param");
                assert(param->nodes.size() == 2);
                std::string param_name{param->nodes.front()->token};
                std::string type{param->nodes.back()->token};
                params.emplace_back(
                    std::move(param_name),
                    ast::UnresolvedTypeRef{std::move(type), to_range(base, *param->nodes.back())});
            }
        }
        auto parsed_return_type = node.nodes[2];
        assert(parsed_return_type->is_token);
        ast::UnresolvedTypeRef return_type{std::string{parsed_return_type->token},
                                           to_range(base, *parsed_return_type)};

        std::vector<std::unique_ptr<ast::Node>> body;
        for (const auto &child : node.nodes | std::views::drop(3)) {
            body.push_back(convert_stmt(*child, base));
        }
        return std::make_unique<ast::FunctionDecl>(std::move(fn_name), std::move(params),
                                                   std::move(return_type), std::move(body));
    }
    return convert(node, base);
}

std::unique_ptr<ast::Module> Parser::parse(std::filesystem::path path, llvm::SourceMgr &source_mgr,
                                           forge::DiagnosticEmitter &emitter) {
    auto buffer = llvm::MemoryBuffer::getFile(path.string());
    if (!buffer) {
        std::println(stderr, "error: cannot open '{}': {}", path.string(),
                     buffer.getError().message());
        return nullptr;
    }

    const char *base = (*buffer)->getBufferStart();
    const size_t size = (*buffer)->getBufferSize();
    unsigned buf_id = source_mgr.AddNewSourceBuffer(std::move(*buffer), llvm::SMLoc{});

    peg::parser peg;
    peg.set_logger([&](size_t ln, size_t col, const std::string &msg, const std::string &rule) {
        std::println(stderr, "grammar error: {}:{}: {} (rule: {})", ln, col, msg, rule);
    });
    if (!peg.load_grammar(kGrammar)) {
        std::println(stderr, "internal error: grammar is invalid");
        return nullptr;
    }
    peg.enable_ast();
    peg.set_logger([&source_mgr, &emitter, buf_id, base,
                    size](size_t ln, size_t col, const std::string &msg, const std::string &) {
        auto loc = source_mgr.FindLocForLineAndColumn(buf_id, static_cast<unsigned>(ln),
                                                      static_cast<unsigned>(col));
        // peglib reports past-EOF positions when input ends prematurely; clamp into range.
        if (size > 0 && (!loc.isValid() || loc.getPointer() >= base + size)) {
            loc = llvm::SMLoc::getFromPointer(base + size - 1);
        }
        if (!loc.isValid()) {
            return;
        }
        auto end_ptr = loc.getPointer() + 1;
        if (end_ptr > base + size) {
            end_ptr = base + size;
        }
        emitter.error(llvm::SMRange{loc, llvm::SMLoc::getFromPointer(end_ptr)}, msg,
                      "syntax error");
    });

    std::shared_ptr<peg::Ast> tree;
    bool ok = peg.parse_n(base, size, tree, path.c_str());
    if (!ok || !tree) {
        return nullptr;
    }

    std::vector<std::unique_ptr<ast::Node>> nodes;
    for (const auto &child : tree->nodes) {
        const auto &stmt = child->nodes[0];
        if (auto node = convert_stmt(*stmt, base)) {
            nodes.push_back(std::move(node));
        }
    }

    auto mod = std::make_unique<ast::Module>(std::move(nodes));
    mod->loc = {llvm::SMLoc::getFromPointer(base), llvm::SMLoc::getFromPointer(base + size)};
    return mod;
}
