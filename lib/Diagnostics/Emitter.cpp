#include "forge/Diagnostics/Emitter.h"
#include "pretty_diagnostics/renderer.hpp"
#include "pretty_diagnostics/report.hpp"
#include "pretty_diagnostics/source.hpp"
#include <cassert>
#include <sstream>

namespace forge {

DiagnosticEmitter::DiagnosticEmitter(llvm::SourceMgr &source_mgr, std::ostream &out)
    : source_mgr_(source_mgr), out_(out) {}

std::shared_ptr<pretty_diagnostics::StringSource>
DiagnosticEmitter::get_source(unsigned buffer_id) {
    auto it = sources_.find(buffer_id);
    if (it != sources_.end()) {
        return it->second;
    }

    auto *mem = source_mgr_.getMemoryBuffer(buffer_id);
    std::string content{mem->getBufferStart(), mem->getBufferEnd()};
    auto source = std::make_shared<pretty_diagnostics::StringSource>(
        content, mem->getBufferIdentifier().str());
    sources_.emplace(buffer_id, source);
    return source;
}

pretty_diagnostics::Span DiagnosticEmitter::make_span(llvm::SMRange range) {
    assert(range.isValid() && "diagnostic span has no source location");
    auto buf_id = source_mgr_.FindBufferContainingLoc(range.Start);
    auto source = get_source(buf_id);

    // LLVM returns 1-based line/col; pretty_diagnostics expects 0-based.
    auto [sl, sc] = source_mgr_.getLineAndColumn(range.Start);
    auto [el, ec] = source_mgr_.getLineAndColumn(range.End);
    // pretty_diagnostics' renderer does `for (col; col < end - 1; ...)` on size_t and
    // underflows when the 0-based end column is 0 — either a zero-width span at col 1
    // or a span that ends at col 1 of a later line. Collapse to a 1-col-wide single-line
    // span anchored at the start position.
    if (sl != el || sc >= ec) {
        el = sl;
        ec = sc + 1;
    }
    return {source, sl - 1, sc - 1, el - 1, ec - 1};
}

void DiagnosticEmitter::error(llvm::SMRange range, std::string_view message, std::string_view label,
                              std::optional<std::string_view> note) {
    ++error_count_;

    auto builder = pretty_diagnostics::Report::Builder()
                       .severity(pretty_diagnostics::Severity::Error)
                       .message(std::string(message))
                       .label(std::string(label), make_span(range));

    if (note) {
        builder.note(std::string(*note));
    }

    const auto report = builder.build();
    auto renderer = pretty_diagnostics::TextRenderer(report);
    std::ostringstream buf;
    report.render(renderer, buf);
    out_ << buf.str();
}

} // namespace forge
