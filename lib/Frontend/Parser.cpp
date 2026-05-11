#include "forge/Frontend/Parser.h"
#include "llvm/Support/MemoryBuffer.h"
#include <peglib.h>
#include <print>

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
            if (op_node.token == "+")
                op_type = ast::BinaryOperator::OpType::Add;
            else if (op_node.token == "-")
                op_type = ast::BinaryOperator::OpType::Sub;
            else if (op_node.token == "*")
                op_type = ast::BinaryOperator::OpType::Mul;
            else
                op_type = ast::BinaryOperator::OpType::Div;
            llvm::SMRange loc{result->loc.Start, rhs->loc.End};
            auto binop =
                std::make_unique<ast::BinaryOperator>(op_type, std::move(result), std::move(rhs));
            binop->loc = loc;
            result = std::move(binop);
        }
        return result;
    }
    // Pass-through: expr, atom, single-child additive
    if (!node.nodes.empty())
        return convert(*node.nodes[0], base);
    return nullptr;
}

static std::unique_ptr<ast::Node> convert_stmt(const peg::Ast &node, const char *base) {
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
    peg.set_logger([&source_mgr, &emitter, buf_id](size_t ln, size_t col, const std::string &msg,
                                                   const std::string &) {
        auto loc = source_mgr.FindLocForLineAndColumn(buf_id, static_cast<unsigned>(ln),
                                                      static_cast<unsigned>(col));
        if (!loc.isValid())
            return;
        emitter.error(llvm::SMRange{loc, loc}, msg, "syntax error");
    });

    std::shared_ptr<peg::Ast> tree;
    bool ok = peg.parse_n(base, size, tree, path.c_str());
    if (!ok || !tree) {
        return nullptr;
    }

    std::vector<std::unique_ptr<ast::Node>> nodes;
    for (const auto &child : tree->nodes) {
        const auto &stmt = child->nodes[0];
        if (auto node = convert_stmt(*stmt, base))
            nodes.push_back(std::move(node));
    }

    auto mod = std::make_unique<ast::Module>(std::move(nodes));
    mod->loc = {llvm::SMLoc::getFromPointer(base), llvm::SMLoc::getFromPointer(base + size)};
    return mod;
}
