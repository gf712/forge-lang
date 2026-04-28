add_library(ForgeConversionASTToMLIR STATIC MLIRGenerator.cpp)
target_link_libraries(ForgeConversionASTToMLIR PUBLIC ForgeAST ForgeIR MLIRArithDialect MLIRDialect
                                                      MLIRIR)
