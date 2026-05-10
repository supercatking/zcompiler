#ifndef ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD
#define ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD

include "zcompiler/Dialect/ZC/IR/ZCDialect.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class ZC_Op<string mnemonic, list<Trait> traits = []>
    : Op<ZC_Dialect, mnemonic, traits>;

def ZC_ConstantOp : ZC_Op<"constant", [Pure]> {
  let summary = "i32 constant";
  let description = [{
    Represents an integer constant in the zc dialect.
  }];
  let arguments = (ins I32Attr:$value);
  let results = (outs I32:$result);
  let assemblyFormat = "$value attr-dict `:` type($result)";
}

def ZC_AddOp : ZC_Op<"add", [Pure]> {
  let summary = "signed i32 addition";
  let arguments = (ins I32:$lhs, I32:$rhs);
  let results = (outs I32:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}

def ZC_SubOp : ZC_Op<"sub", [Pure]> {
  let summary = "signed i32 subtraction";
  let arguments = (ins I32:$lhs, I32:$rhs);
  let results = (outs I32:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}

def ZC_MulOp : ZC_Op<"mul", [Pure]> {
  let summary = "signed i32 multiplication";
  let arguments = (ins I32:$lhs, I32:$rhs);
  let results = (outs I32:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}

def ZC_DivOp : ZC_Op<"div", [Pure]> {
  let summary = "signed i32 division";
  let arguments = (ins I32:$lhs, I32:$rhs);
  let results = (outs I32:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}

#endif // ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD
