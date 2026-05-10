#ifndef ZCOMPILER_TARGET_RISCV_RISCVBACKEND_H
#define ZCOMPILER_TARGET_RISCV_RISCVBACKEND_H

#include "zcompiler/AST/AST.h"
#include "zcompiler/CodeGen/CodeGen.h"

#include "llvm/Support/raw_ostream.h"

namespace zc {

CodeGenResult emitLLVMIRWithMLIRPipeline(const ModuleAST &module,
                                         llvm::raw_ostream &os);

CodeGenResult emitRiscVAssemblyWithLLVMBackend(const ModuleAST &module,
                                               llvm::raw_ostream &os);

} // namespace zc

#endif // ZCOMPILER_TARGET_RISCV_RISCVBACKEND_H
