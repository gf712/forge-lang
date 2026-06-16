include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "ForgeBase.td"

class ForgeAttr<string name, string attrMnemonic, list<Trait> traits = [],
                string baseCppClass = "::mlir::Attribute">
  : AttrDef<Forge_Dialect, name, traits, baseCppClass> {
  let mnemonic = attrMnemonic;
}

def Forge_ComptimeIntAttr : ForgeAttr<"ComptimeInt", "comptime_int",
                [DeclareAttrInterfaceMethods<TypedAttrInterface>]> {
    let parameters = (ins APIntParameter<"">:$value);
    let assemblyFormat = "`<` $value `>`";
}

def Forge_ComptimeFloatAttr : ForgeAttr<"ComptimeFloat", "comptime_float",
                [DeclareAttrInterfaceMethods<TypedAttrInterface>]> {
    let parameters = (ins APFloatParameter<"">:$value);
    let hasCustomAssemblyFormat = 1;
}
