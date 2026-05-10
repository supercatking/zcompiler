#ifndef ZCOMPILER_DIALECT_ZC_IR_ZCOPS_H
#define ZCOMPILER_DIALECT_ZC_IR_ZCOPS_H

#include "zcompiler/Dialect/ZC/IR/ZCDialect.h"

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "zcompiler/Dialect/ZC/IR/ZCOps.h.inc"

#endif // ZCOMPILER_DIALECT_ZC_IR_ZCOPS_H
