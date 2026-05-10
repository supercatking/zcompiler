#include "zcompiler/Conversion/ZCToStandard/ZCToStandard.h"

#include "zcompiler/Dialect/ZC/IR/ZCOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/TypeSwitch.h"

#include <memory>

using namespace mlir;

namespace zc {
namespace {

class LowerZCToStandardPass
    : public PassWrapper<LowerZCToStandardPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerZCToStandardPass)

  StringRef getArgument() const final { return "lower-zc-to-standard"; }
  StringRef getDescription() const final {
    return "Lower zc dialect arithmetic operations to arith dialect";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<arith::ArithDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    SmallVector<Operation *> operations;
    module.walk([&](Operation *operation) {
      if (operation->getDialect()->getNamespace() == "zc")
        operations.push_back(operation);
    });

    for (Operation *operation : operations)
      lowerOperation(operation);
  }

private:
  void lowerOperation(Operation *operation) {
    OpBuilder builder(operation);
    Location loc = operation->getLoc();

    TypeSwitch<Operation *>(operation)
        .Case<mlir::zc::ConstantOp>([&](mlir::zc::ConstantOp op) {
          Value replacement = builder.create<arith::ConstantIntOp>(
              loc, static_cast<int64_t>(op.getValue()), 32);
          replace(operation, replacement);
        })
        .Case<mlir::zc::AddOp>([&](mlir::zc::AddOp op) {
          replace(operation,
                  builder.create<arith::AddIOp>(loc, op.getLhs(), op.getRhs()));
        })
        .Case<mlir::zc::SubOp>([&](mlir::zc::SubOp op) {
          replace(operation,
                  builder.create<arith::SubIOp>(loc, op.getLhs(), op.getRhs()));
        })
        .Case<mlir::zc::MulOp>([&](mlir::zc::MulOp op) {
          replace(operation,
                  builder.create<arith::MulIOp>(loc, op.getLhs(), op.getRhs()));
        })
        .Case<mlir::zc::DivOp>([&](mlir::zc::DivOp op) {
          replace(operation, builder.create<arith::DivSIOp>(loc, op.getLhs(),
                                                            op.getRhs()));
        })
        .Default([&](Operation *unknown) {
          unknown->emitError("unsupported zc operation for lowering");
          signalPassFailure();
        });
  }

  void replace(Operation *operation, Value replacement) {
    operation->getResult(0).replaceAllUsesWith(replacement);
    operation->erase();
  }
};

} // namespace

void registerZCToStandardPasses() {
  PassRegistration<LowerZCToStandardPass>();
}

} // namespace zc
