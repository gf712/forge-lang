add_executable(forge main.cpp)
target_link_libraries(
    forge
    PRIVATE ForgeParser
            ForgeSymbolResolver
            ForgeTypeChecker
            ForgeDiagnostics
            ForgeConversionASTToMLIR
            ForgeConversionMLIRToLLVM
            ForgeTargetJIT
            ForgeMLIRDialect)
