#include "forge/Dialect/Forge/ForgeDialect.h"

#include "forge/Dialect/Forge/ForgeAttributes.h"
#include "forge/Dialect/Forge/ForgeTypes.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"
#include <llvm/ADT/APFloat.h>

#include "forge/Dialect/Forge/ForgeDialect.cpp.inc"

#define GET_ATTRDEF_CLASSES
#include "forge/Dialect/Forge/ForgeAttributes.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "forge/Dialect/Forge/ForgeTypes.cpp.inc"

using namespace mlir;
using namespace mlir::forge;

::mlir::Attribute mlir::forge::ComptimeFloatAttr::parse(mlir::AsmParser &parser, mlir::Type type) {
    double result;
    if (parser.parseLess() || parser.parseFloat(result) || parser.parseGreater()) {
        return {};
    }
    return mlir::forge::ComptimeFloatAttr::get(parser.getContext(), llvm::APFloat{result});
}

::mlir::Type mlir::forge::ComptimeFloatAttr::getType() const {
    return mlir::forge::ComptimeFloatType::get(getContext());
}

void mlir::forge::ComptimeFloatAttr::print(mlir::AsmPrinter &printer) const {
    printer << "<";
    printer.printFloat(getValue());
    printer << ">";
}

::mlir::Type mlir::forge::ComptimeIntAttr::getType() const {
    return mlir::forge::ComptimeIntType::get(getContext());
}

void ForgeDialect::initialize() {
    addAttributes<
#define GET_ATTRDEF_LIST
#include "forge/Dialect/Forge/ForgeAttributes.cpp.inc"
        >();
    addTypes<
#define GET_TYPEDEF_LIST
#include "forge/Dialect/Forge/ForgeTypes.cpp.inc"
        >();
}
