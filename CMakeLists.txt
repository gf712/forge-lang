cmake_minimum_required(VERSION 4.0.0)

include(FetchContent)
set(CMAKE_CXX_STANDARD 26)

set(PROJECT_OPTIONS_VERSION "v0.41.1")
FetchContent_Declare(
    _project_options
    URL https://github.com/aminya/project_options/archive/refs/tags/${PROJECT_OPTIONS_VERSION}.zip)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)

project(ForgeLang LANGUAGES CXX)
find_package(LLVM CONFIG REQUIRED)
message("LLVM_INSTALL_PREFIX: ${LLVM_INSTALL_PREFIX}")
find_package(MLIR CONFIG REQUIRED PATHS ${LLVM_INSTALL_PREFIX}/lib/cmake/mlir/)

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(TableGen)
include(AddLLVM)
include(AddMLIR)

project_options(
    PREFIX
    "myproject"
    ENABLE_CACHE
    # ENABLE_CPPCHECK ENABLE_CLANG_TIDY ENABLE_VS_ANALYSIS ENABLE_INTERPROCEDURAL_OPTIMIZATION
    # ENABLE_NATIVE_OPTIMIZATION ${ENABLE_DOXYGEN} ${ENABLE_COVERAGE} ${ENABLE_SANITIZER_ADDRESS}
    # ${ENABLE_SANITIZER_UNDEFINED} ${ENABLE_SANITIZER_THREAD} ${ENABLE_SANITIZER_MEMORY}
    # ENABLE_SANITIZER_POINTER_COMPARE ENABLE_SANITIZER_POINTER_SUBTRACT
    # ENABLE_CONTROL_FLOW_PROTECTION ENABLE_STACK_PROTECTION ENABLE_OVERFLOW_PROTECTION
    # ENABLE_ELF_PROTECTION ENABLE_RUNTIME_SYMBOLS_RESOLUTION ENABLE_COMPILE_COMMANDS_SYMLINK
    # ENABLE_PCH PCH_HEADERS WARNINGS_AS_ERRORS ENABLE_INCLUDE_WHAT_YOU_USE ENABLE_GCC_ANALYZER
    # ENABLE_BUILD_WITH_TIME_TRACE ENABLE_UNITY LINKER "lld"
)

set(BUILD_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_EXECUTABLE
    OFF
    CACHE BOOL "" FORCE)
FetchContent_Declare(
    pretty_diagnostics
    GIT_REPOSITORY https://github.com/Excse/pretty_diagnostics.git
    GIT_TAG main)
FetchContent_MakeAvailable(pretty_diagnostics)

FetchContent_Declare(
    cpp_peglib
    GIT_REPOSITORY https://github.com/yhirose/cpp-peglib
    GIT_TAG v1.9.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cpp_peglib)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(tests)
