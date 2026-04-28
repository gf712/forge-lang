add_executable(forge main.cpp)
target_link_libraries(
    forge PRIVATE ForgeParser ForgeTypeChecker ForgeDiagnostics ForgeConversionASTToMLIR
                  ForgeConversionMLIRToLLVM ForgeTargetJIT)
