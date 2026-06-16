import lit.formats
import os

config.name        = "Forge"
config.test_format = lit.formats.ShTest(execute_external=False)
config.suffixes    = [".fg", ".mlir"]
config.excludes    = ["Inputs"]

# %forge-opt must precede %forge: lit does substring substitution, so the
# shorter %forge would otherwise match inside %forge-opt.
config.substitutions += [
    ("%forge-opt", config.forge_opt_binary),
    ("%forge",     config.forge_binary),
    ("%FileCheck", config.filecheck_binary),
]

config.environment["PATH"] = os.pathsep.join([
    os.path.dirname(config.forge_binary),
    config.llvm_tools_dir,
    config.environment.get("PATH", ""),
])
