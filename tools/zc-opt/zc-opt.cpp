#include "zcompiler/Conversion/ZCToStandard/ZCToStandard.h"
#include "zcompiler/Dialect/ZC/IR/ZCDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
  mlir::registerAllPasses();
  zc::registerZCToStandardPasses();

  mlir::DialectRegistry registry;
  registry.insert<mlir::zc::ZCDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "zcompiler optimizer driver\n", registry));
}
