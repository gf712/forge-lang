add_mlir_dialect_library(
    ForgeMLIRDialect
    ForgeDialect.cpp
    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/forge/Dialect/Forge
    DEPENDS
    MLIRForgeOpsIncGen
    LINK_LIBS
    PUBLIC
    MLIRIR)
target_include_directories(
    ForgeMLIRDialect PUBLIC ${CMAKE_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
                            ${MLIR_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
