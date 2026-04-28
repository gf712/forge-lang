add_library(ForgeTargetJIT STATIC JIT.cpp)
target_link_libraries(
    ForgeTargetJIT PUBLIC ForgeIR MLIRExecutionEngine MLIRTargetLLVMIRExport
                          MLIRBuiltinToLLVMIRTranslation MLIRLLVMToLLVMIRTranslation)
