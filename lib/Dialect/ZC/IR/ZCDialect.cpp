#include "zcompiler/Dialect/ZC/IR/ZCDialect.h"
#include "zcompiler/Dialect/ZC/IR/ZCOps.h"

using namespace mlir;
using namespace mlir::zc;

#include "zcompiler/Dialect/ZC/IR/ZCOpsDialect.cpp.inc"

void ZCDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "zcompiler/Dialect/ZC/IR/ZCOps.cpp.inc"
      >();
}

