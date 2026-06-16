include "ForgeBase.td"
include "mlir/IR/AttrTypeBase.td"

class Forge_Type<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<Forge_Dialect, name, traits> {
  let mnemonic = typeMnemonic;
}

def Forge_ComptimeIntType: Forge_Type<"ComptimeInt", "comptime_int"> {
  
}


def Forge_ComptimeFloatType: Forge_Type<"ComptimeFloat", "comptime_float"> {
  
}
