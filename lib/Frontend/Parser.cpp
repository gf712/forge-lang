#include "forge/Frontend/Parser.h"
#include "llvm/Support/MemoryBuffer.h"
#include <print>

std::unique_ptr<ast::Module> Parser::parse(std::filesystem::path path,
                                           llvm::SourceMgr &source_mgr) {
    auto buffer = llvm::MemoryBuffer::getFile(path.string());
    if (!buffer) {
        std::println(stderr, "error: cannot open '{}': {}", path.string(),
                     buffer.getError().message());
        return nullptr;
    }

    const char *buf_start = (*buffer)->getBufferStart();
    source_mgr.AddNewSourceBuffer(std::move(*buffer), llvm::SMLoc{});

    // Stub: every node gets the same location (start of file) until the lexer is real.
    llvm::SMLoc loc = llvm::SMLoc::getFromPointer(buf_start);
    llvm::SMRange range{loc, loc};

    using namespace ast;
    auto lhs = std::make_unique<IntLiteral>(1);
    lhs->loc = range;
    auto rhs = std::make_unique<IntLiteral>(2);
    rhs->loc = range;

    auto binop = std::make_unique<BinaryOperator>(BinaryOperator::OpType::Add, std::move(lhs),
                                                  std::move(rhs));
    binop->loc = range;

    std::vector<std::unique_ptr<Expression>> nodes;
    nodes.push_back(std::move(binop));

    auto mod = std::make_unique<Module>(std::move(nodes));
    mod->loc = range;
    return mod;
}
