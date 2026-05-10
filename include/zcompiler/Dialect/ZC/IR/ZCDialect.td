#ifndef ZCOMPILER_DIALECT_ZC_IR_ZCDIALECT_TD
#define ZCOMPILER_DIALECT_ZC_IR_ZCDIALECT_TD

include "mlir/IR/OpBase.td"

def ZC_Dialect : Dialect {
  let name = "zc";
  let summary = "zcompiler toy dialect";
  let description = [{
    The zc dialect is the first project-owned dialect for zcompiler. It models
    the tiny source language before lowering into standard MLIR func and arith
    dialect operations.
  }];
  let cppNamespace = "::zc";
}

#endif // ZCOMPILER_DIALECT_ZC_IR_ZCDIALECT_TD

