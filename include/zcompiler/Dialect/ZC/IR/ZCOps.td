#ifndef ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD
#define ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD

include "zcompiler/Dialect/ZC/IR/ZCDialect.td"

class ZC_Op<string mnemonic, list<Trait> traits = []>
    : Op<ZC_Dialect, mnemonic, traits>;

def ZC_ConstantOp : ZC_Op<"constant", [Pure]> {
  let summary = "i32 constant";
  let description = [{
    Represents an integer constant in the zc dialect.
  }];
}

def ZC_AddOp : ZC_Op<"add", [Pure]> {
  let summary = "signed i32 addition";
}

def ZC_SubOp : ZC_Op<"sub", [Pure]> {
  let summary = "signed i32 subtraction";
}

def ZC_MulOp : ZC_Op<"mul", [Pure]> {
  let summary = "signed i32 multiplication";
}

def ZC_DivOp : ZC_Op<"div", [Pure]> {
  let summary = "signed i32 division";
}

def ZC_ReturnOp : ZC_Op<"return", [Terminator]> {
  let summary = "return from zc function";
}

def ZC_FuncOp : ZC_Op<"func", [IsolatedFromAbove]> {
  let summary = "zc function";
}

#endif // ZCOMPILER_DIALECT_ZC_IR_ZCOPS_TD

