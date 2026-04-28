add_library(ForgeAST INTERFACE)
target_include_directories(ForgeAST INTERFACE ${CMAKE_SOURCE_DIR}/include ${MLIR_INCLUDE_DIRS}
                                              ${LLVM_INCLUDE_DIRS})
target_link_libraries(ForgeAST INTERFACE MLIRIR)

add_library(ForgeParser STATIC Parser.cpp)
target_link_libraries(ForgeParser PUBLIC ForgeAST peglib)

add_library(ForgeTypeChecker STATIC TypeChecker.cpp)
target_link_libraries(ForgeTypeChecker PUBLIC ForgeAST MLIRIR)
