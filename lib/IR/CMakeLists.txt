add_library(ForgeIR INTERFACE)
target_include_directories(ForgeIR INTERFACE ${CMAKE_SOURCE_DIR}/include ${MLIR_INCLUDE_DIRS}
                                             ${LLVM_INCLUDE_DIRS})
target_link_libraries(ForgeIR INTERFACE MLIRIR)
