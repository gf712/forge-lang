#ifndef FORGE_BASE
#define FORGE_BASE

include "mlir/IR/DialectBase.td"

def Forge_Dialect : Dialect {
    let name = "forge";

    let summary = "forge dialect";

    let description = [{
TODO
    }];

    let cppNamespace = "::mlir::forge";

    let useDefaultAttributePrinterParser = 1;
    
    let useDefaultTypePrinterParser = 1;
}

#endif // FORGE_BASE
