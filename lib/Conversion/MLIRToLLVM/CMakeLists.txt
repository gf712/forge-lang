add_library(ForgeConversionMLIRToLLVM STATIC MLIRToLLVM.cpp)
target_link_libraries(
    ForgeConversionMLIRToLLVM
    PUBLIC ForgeIR
           MLIRLLVMCommonConversion
           MLIRArithToLLVM
           MLIRArithTransforms
           MLIRControlFlowToLLVM
           MLIRControlFlowDialect
           MLIRFuncToLLVM
           MLIRDialect
           MLIRIR)
