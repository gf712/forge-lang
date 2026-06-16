#include "forge/Dialect/Forge/ForgeDialect.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

// A thin mlir-opt driver with only the Forge dialect registered. Its job is to
// parse and re-print MLIR so dialect types/attributes can be roundtrip-tested
// (`forge-opt %s | FileCheck %s`) without a frontend path reaching them yet.
int main(int argc, char **argv) {
    mlir::DialectRegistry registry;
    registry.insert<mlir::forge::ForgeDialect>();
    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "Forge MLIR roundtrip / optimizer driver\n", registry));
}
