set(LLVM_TARGET_DEFINITIONS ForgeBase.td)
mlir_tablegen(ForgeDialect.h.inc -gen-dialect-decls -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})
mlir_tablegen(ForgeDialect.cpp.inc -gen-dialect-defs -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})

set(LLVM_TARGET_DEFINITIONS ForgeAttributes.td)
mlir_tablegen(ForgeAttributes.h.inc -gen-attrdef-decls -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})
mlir_tablegen(ForgeAttributes.cpp.inc -gen-attrdef-defs -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})

set(LLVM_TARGET_DEFINITIONS ForgeTypes.td)
mlir_tablegen(ForgeTypes.h.inc -gen-typedef-decls -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})
mlir_tablegen(ForgeTypes.cpp.inc -gen-typedef-defs -dialect=forge EXTRA_INCLUDES
              ${MLIR_INCLUDE_DIRS})

add_public_tablegen_target(MLIRForgeOpsIncGen)
