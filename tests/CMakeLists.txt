find_program(
    FILECHECK_BINARY FileCheck
    PATHS "${LLVM_TOOLS_BINARY_DIR}" /usr/lib/llvm-23/bin
    NO_DEFAULT_PATH REQUIRED)

find_program(
    LIT_BINARY lit
    PATHS "${LLVM_TOOLS_BINARY_DIR}" /usr/lib/llvm-23/bin
    NO_DEFAULT_PATH REQUIRED)

set(FORGE_BINARY "${CMAKE_BINARY_DIR}/tools/forge/forge")

configure_file(lit.site.cfg.py.in ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py @ONLY)

add_custom_target(
    check-forge
    COMMAND ${LIT_BINARY} -v ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS forge
    COMMENT "Running Forge regression tests"
    USES_TERMINAL)
