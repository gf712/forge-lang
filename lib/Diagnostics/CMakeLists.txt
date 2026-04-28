add_library(ForgeDiagnostics STATIC Emitter.cpp)
target_include_directories(ForgeDiagnostics PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(
    ForgeDiagnostics
    PUBLIC ForgeAST
    PRIVATE pretty_diagnostics::pretty_diagnostics)
