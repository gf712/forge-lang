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
    if (node.name == "additive" && node.nodes.size() > 1) {
        auto result = convert(*node.nodes[0], base);
        for (size_t i = 1; i < node.nodes.size(); ++i) {
            auto rhs = convert(*node.nodes[i], base);
            llvm::SMRange loc{result->loc.Start, rhs->loc.End};
            auto binop = std::make_unique<ast::BinaryOperator>(ast::BinaryOperator::OpType::Add,
                                                               std::move(result), std::move(rhs));
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

std::unique_ptr<ast::Module> Parser::parse(std::filesystem::path path,
                                           llvm::SourceMgr &source_mgr) {
    auto buffer = llvm::MemoryBuffer::getFile(path.string());
    if (!buffer) {
        std::println(stderr, "error: cannot open '{}': {}", path.string(),
                     buffer.getError().message());
        return nullptr;
    }

    const char *base = (*buffer)->getBufferStart();
    const size_t size = (*buffer)->getBufferSize();
    source_mgr.AddNewSourceBuffer(std::move(*buffer), llvm::SMLoc{});

    peg::parser peg(k_grammar);
    if (!peg) {
        std::println(stderr, "internal error: grammar is invalid");
        return nullptr;
    }
    peg.enable_ast();
    peg.set_logger([&path](size_t ln, size_t col, const std::string &msg, const std::string &) {
        std::println(stderr, "{}:{}:{}: error: {}", path.string(), ln, col, msg);
    });

    std::shared_ptr<peg::Ast> tree;
    bool ok = peg.parse_n(base, size, tree, path.c_str());
    if (!ok || !tree) {
        return nullptr;
    }

    std::vector<std::unique_ptr<ast::Expression>> nodes;
    for (auto &child : tree->nodes)
        if (auto expr = convert(*child, base))
            nodes.push_back(std::move(expr));

    auto mod = std::make_unique<ast::Module>(std::move(nodes));
    mod->loc = {llvm::SMLoc::getFromPointer(base), llvm::SMLoc::getFromPointer(base + size)};
    return mod;
}
