#ifndef ZCOMPILER_MLIRGEN_MLIRGEN_H
#define ZCOMPILER_MLIRGEN_MLIRGEN_H

#include "zcompiler/AST/AST.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"

#include <string>
#include <vector>

namespace zc {

class MLIRGenResult {
public:
  bool succeeded() const { return diagnostics.empty(); }
  void addDiagnostic(std::string diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
  }
  const std::vector<std::string> &getDiagnostics() const { return diagnostics; }

private:
  std::vector<std::string> diagnostics;
};

mlir::OwningOpRef<mlir::ModuleOp> generateMLIRModule(
    mlir::MLIRContext &context, const ModuleAST &module,
    MLIRGenResult &result);

} // namespace zc

#endif // ZCOMPILER_MLIRGEN_MLIRGEN_H

