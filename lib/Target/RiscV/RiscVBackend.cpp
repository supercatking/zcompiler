#include "zcompiler/Target/RiscV/RiscVBackend.h"

#include "zcompiler/MLIRGen/MLIRGen.h"

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <optional>
#include <string>

using namespace llvm;

namespace zc {
namespace {

constexpr StringRef kMLIROptPath = "/home/zyz/mlir/build/bin/mlir-opt";
constexpr StringRef kMLIRTranslatePath =
    "/home/zyz/mlir/build/bin/mlir-translate";
constexpr StringRef kSystemLLCPath = "/usr/bin/llc";
constexpr StringRef kMLIRBuildLLCPath = "/home/zyz/mlir/build/bin/llc";

void addDiagnostic(CodeGenResult &result, const Twine &message) {
  SmallString<128> buffer;
  message.toVector(buffer);
  result.addDiagnostic(std::string(buffer.str()));
}

bool writeStandardMLIRToFile(const ModuleAST &module, StringRef path,
                             CodeGenResult &result) {
  mlir::MLIRContext context;
  MLIRGenResult mlirGenResult;
  mlir::OwningOpRef<mlir::ModuleOp> mlirModule =
      generateMLIRModule(context, module, mlirGenResult);
  if (!mlirGenResult.succeeded()) {
    for (const std::string &diagnostic : mlirGenResult.getDiagnostics())
      result.addDiagnostic(diagnostic);
    return false;
  }

  std::error_code ec;
  raw_fd_ostream mlirFile(path, ec, sys::fs::OF_Text);
  if (ec) {
    addDiagnostic(result, "failed to create temporary MLIR file: " +
                              Twine(ec.message()));
    return false;
  }

  mlirModule->print(mlirFile);
  mlirFile << "\n";
  return true;
}

bool runMLIRToLLVMIR(StringRef mlirInputPath, StringRef llvmDialectPath,
                     StringRef llvmIRPath, CodeGenResult &result) {
  if (!sys::fs::can_execute(kMLIROptPath)) {
    addDiagnostic(result, "mlir-opt is not executable at " + kMLIROptPath);
    return false;
  }
  if (!sys::fs::can_execute(kMLIRTranslatePath)) {
    addDiagnostic(result,
                  "mlir-translate is not executable at " + kMLIRTranslatePath);
    return false;
  }

  std::optional<StringRef> redirects[3] = {std::nullopt, std::nullopt,
                                           std::nullopt};
  SmallVector<StringRef, 8> mlirOptArgs = {
      kMLIROptPath,
      mlirInputPath,
      "--convert-to-llvm",
      "--reconcile-unrealized-casts",
      "-o",
      llvmDialectPath,
  };
  if (sys::ExecuteAndWait(kMLIROptPath, mlirOptArgs, std::nullopt, redirects) !=
      0) {
    result.addDiagnostic("mlir-opt failed to lower MLIR to the LLVM dialect");
    return false;
  }

  SmallVector<StringRef, 6> translateArgs = {
      kMLIRTranslatePath,
      "--mlir-to-llvmir",
      llvmDialectPath,
      "-o",
      llvmIRPath,
  };
  if (sys::ExecuteAndWait(kMLIRTranslatePath, translateArgs, std::nullopt,
                          redirects) != 0) {
    result.addDiagnostic("mlir-translate failed to emit LLVM IR");
    return false;
  }

  return true;
}

bool createTemporaryFile(StringRef prefix, StringRef suffix,
                         SmallVectorImpl<char> &path,
                         CodeGenResult &result) {
  if (!sys::fs::createTemporaryFile(prefix, suffix, path))
    return true;

  addDiagnostic(result, "failed to create temporary " + suffix + " file");
  return false;
}

CodeGenResult emitLLVMIRToFile(const ModuleAST &module, StringRef llvmIRPath) {
  CodeGenResult result;
  SmallString<128> mlirInputPath;
  SmallString<128> llvmDialectPath;
  if (!createTemporaryFile("zcompiler-input", "mlir", mlirInputPath, result) ||
      !createTemporaryFile("zcompiler-llvm-dialect", "mlir", llvmDialectPath,
                           result))
    return result;

  FileRemover removeMLIRInput(mlirInputPath);
  FileRemover removeLLVMDialect(llvmDialectPath);

  if (!writeStandardMLIRToFile(module, mlirInputPath, result))
    return result;
  runMLIRToLLVMIR(mlirInputPath, llvmDialectPath, llvmIRPath, result);
  return result;
}

CodeGenResult emitReferenceLLVMIRToFile(const ModuleAST &module,
                                        StringRef llvmIRPath) {
  CodeGenResult result;
  std::error_code ec;
  raw_fd_ostream llvmFile(llvmIRPath, ec, sys::fs::OF_Text);
  if (ec) {
    addDiagnostic(result, "failed to create temporary LLVM IR file: " +
                              Twine(ec.message()));
    return result;
  }

  result = emitLLVMIR(module, llvmFile);
  llvmFile << "\n";
  return result;
}

std::optional<StringRef> findLLC() {
  if (sys::fs::can_execute(kSystemLLCPath))
    return kSystemLLCPath;
  if (sys::fs::can_execute(kMLIRBuildLLCPath))
    return kMLIRBuildLLCPath;
  return std::nullopt;
}

bool runLLC(StringRef llcPath, StringRef llvmIRPath, StringRef assemblyPath,
            CodeGenResult &result) {
  StringRef devNull = "/dev/null";
  std::optional<StringRef> redirects[3] = {std::nullopt, std::nullopt,
                                           devNull};
  SmallVector<StringRef, 10> llcArgs = {
      llcPath,
      "-mtriple=riscv64-unknown-elf",
      "-mattr=+m",
      "-filetype=asm",
      llvmIRPath,
      "-o",
      assemblyPath,
  };
  if (sys::ExecuteAndWait(llcPath, llcArgs, std::nullopt, redirects) == 0)
    return true;

  result.addDiagnostic("llc failed to emit RISC-V assembly");
  return false;
}

} // namespace

CodeGenResult emitLLVMIRWithMLIRPipeline(const ModuleAST &module,
                                         raw_ostream &os) {
  CodeGenResult result;
  SmallString<128> llvmIRPath;
  if (!createTemporaryFile("zcompiler-output", "ll", llvmIRPath, result))
    return result;

  FileRemover removeLLVMIR(llvmIRPath);
  result = emitLLVMIRToFile(module, llvmIRPath);
  if (!result.succeeded())
    return result;

  ErrorOr<std::unique_ptr<MemoryBuffer>> llvmIR =
      MemoryBuffer::getFile(llvmIRPath);
  if (!llvmIR) {
    addDiagnostic(result, "failed to read generated LLVM IR: " +
                              Twine(llvmIR.getError().message()));
    return result;
  }

  os << llvmIR.get()->getBuffer();
  return result;
}

CodeGenResult emitRiscVAssemblyWithLLVMBackend(const ModuleAST &module,
                                               raw_ostream &os) {
  CodeGenResult result;
  SmallString<128> llvmIRPath;
  SmallString<128> assemblyPath;
  if (!createTemporaryFile("zcompiler-riscv-input", "ll", llvmIRPath,
                           result) ||
      !createTemporaryFile("zcompiler-riscv-output", "s", assemblyPath,
                           result))
    return result;

  FileRemover removeLLVMIR(llvmIRPath);
  FileRemover removeAssembly(assemblyPath);

  std::optional<StringRef> llcPath = findLLC();
  if (!llcPath) {
    result.addDiagnostic("llc with RISC-V target support was not found");
    return result;
  }

  CodeGenResult mlirLLVMResult = emitLLVMIRToFile(module, llvmIRPath);
  CodeGenResult llcResult;
  if (!mlirLLVMResult.succeeded() ||
      !runLLC(*llcPath, llvmIRPath, assemblyPath, llcResult)) {
    CodeGenResult referenceLLVMResult =
        emitReferenceLLVMIRToFile(module, llvmIRPath);
    if (!referenceLLVMResult.succeeded())
      return referenceLLVMResult;

    CodeGenResult referenceLLCResult;
    if (!runLLC(*llcPath, llvmIRPath, assemblyPath, referenceLLCResult))
      return referenceLLCResult;
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> assembly =
      MemoryBuffer::getFile(assemblyPath);
  if (!assembly) {
    addDiagnostic(result, "failed to read generated RISC-V assembly: " +
                              Twine(assembly.getError().message()));
    return result;
  }

  os << assembly.get()->getBuffer();
  return result;
}

} // namespace zc
