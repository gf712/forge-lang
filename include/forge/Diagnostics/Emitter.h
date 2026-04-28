#pragma once
#include "llvm/Support/SourceMgr.h"
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <unordered_map>

// Forward-declare pretty_diagnostics types to keep this header light.
namespace pretty_diagnostics {
class StringSource;
class Span;
} // namespace pretty_diagnostics

namespace forge {

class DiagnosticEmitter {
  public:
    explicit DiagnosticEmitter(llvm::SourceMgr &source_mgr, std::ostream &out = std::cerr);

    void error(llvm::SMRange range, std::string_view message, std::string_view label,
               std::optional<std::string_view> note = std::nullopt);

    bool has_errors() const { return error_count_ > 0; }

  private:
    llvm::SourceMgr &source_mgr_;
    std::ostream &out_;
    int error_count_ = 0;

    std::unordered_map<unsigned, std::shared_ptr<pretty_diagnostics::StringSource>> sources_;

    std::shared_ptr<pretty_diagnostics::StringSource> get_source(unsigned buffer_id);
    pretty_diagnostics::Span make_span(llvm::SMRange range);
};

} // namespace forge
