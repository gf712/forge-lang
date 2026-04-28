#pragma once
#include "forge/Frontend/AST.h"
#include "llvm/Support/SourceMgr.h"
#include <filesystem>
#include <memory>

struct Parser {
    static std::unique_ptr<ast::Module> parse(std::filesystem::path path,
                                              llvm::SourceMgr &source_mgr);
};
