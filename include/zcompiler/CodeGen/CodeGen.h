#ifndef ZCOMPILER_CODEGEN_CODEGEN_H
#define ZCOMPILER_CODEGEN_CODEGEN_H

#include "zcompiler/AST/AST.h"

#include "llvm/Support/raw_ostream.h"

#include <string>
#include <utility>
#include <vector>

namespace zc {

class CodeGenResult {
public:
  bool succeeded() const { return diagnostics.empty(); }
  void addDiagnostic(std::string diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
  }
  const std::vector<std::string> &getDiagnostics() const { return diagnostics; }

private:
  std::vector<std::string> diagnostics;
};

CodeGenResult emitStandardMLIR(const ModuleAST &module, llvm::raw_ostream &os);
CodeGenResult emitZCMLIR(const ModuleAST &module, llvm::raw_ostream &os);
CodeGenResult emitLoweredMLIR(const ModuleAST &module, llvm::raw_ostream &os);
CodeGenResult emitLLVMIR(const ModuleAST &module, llvm::raw_ostream &os);
CodeGenResult emitRiscVAssembly(const ModuleAST &module, llvm::raw_ostream &os);

} // namespace zc

#endif // ZCOMPILER_CODEGEN_CODEGEN_H
