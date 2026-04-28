import lit.formats
import os

config.name        = "Forge"
config.test_format = lit.formats.ShTest(execute_external=False)
config.suffixes    = [".fg"]
config.excludes    = ["Inputs"]

config.substitutions += [
    ("%forge",     config.forge_binary),
    ("%FileCheck", config.filecheck_binary),
]

config.environment["PATH"] = os.pathsep.join([
    os.path.dirname(config.forge_binary),
    config.llvm_tools_dir,
    config.environment.get("PATH", ""),
])
