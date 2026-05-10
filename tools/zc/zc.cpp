#include "zcompiler/CodeGen/CodeGen.h"
#include "zcompiler/Lexer/Lexer.h"
#include "zcompiler/MLIRGen/MLIRGen.h"
#include "zcompiler/Parser/Parser.h"

#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>

using namespace llvm;

namespace {

bool emitLLVMViaMLIRPipeline(const zc::ModuleAST &module, raw_ostream &output) {
  mlir::MLIRContext context;
  zc::MLIRGenResult mlirGenResult;
  mlir::OwningOpRef<mlir::ModuleOp> mlirModule =
      zc::generateMLIRModule(context, module, mlirGenResult);
  if (!mlirGenResult.succeeded())
    return false;

  SmallString<128> mlirInputPath;
  SmallString<128> llvmDialectPath;
  SmallString<128> llvmIRPath;
  if (sys::fs::createTemporaryFile("zcompiler-input", "mlir", mlirInputPath) ||
      sys::fs::createTemporaryFile("zcompiler-llvm-dialect", "mlir",
                                   llvmDialectPath) ||
      sys::fs::createTemporaryFile("zcompiler-output", "ll", llvmIRPath))
    return false;

  FileRemover removeMLIRInput(mlirInputPath);
  FileRemover removeLLVMDialect(llvmDialectPath);
  FileRemover removeLLVMIR(llvmIRPath);

  std::error_code ec;
  raw_fd_ostream mlirFile(mlirInputPath, ec, sys::fs::OF_Text);
  if (ec)
    return false;
  mlirModule->print(mlirFile);
  mlirFile << "\n";
  mlirFile.close();

  std::string mlirOpt = "/home/zyz/mlir/build/bin/mlir-opt";
  std::string mlirTranslate = "/home/zyz/mlir/build/bin/mlir-translate";
  if (!sys::fs::can_execute(mlirOpt) || !sys::fs::can_execute(mlirTranslate))
    return false;

  std::optional<StringRef> redirects[3] = {std::nullopt, std::nullopt,
                                           std::nullopt};
  SmallVector<StringRef, 8> mlirOptArgs = {
      mlirOpt,
      mlirInputPath,
      "--convert-to-llvm",
      "--reconcile-unrealized-casts",
      "-o",
      llvmDialectPath,
  };
  if (sys::ExecuteAndWait(mlirOpt, mlirOptArgs, std::nullopt, redirects) != 0)
    return false;

  SmallVector<StringRef, 6> translateArgs = {
      mlirTranslate,
      "--mlir-to-llvmir",
      llvmDialectPath,
      "-o",
      llvmIRPath,
  };
  if (sys::ExecuteAndWait(mlirTranslate, translateArgs, std::nullopt,
                          redirects) != 0)
    return false;

  ErrorOr<std::unique_ptr<MemoryBuffer>> llvmIR =
      MemoryBuffer::getFile(llvmIRPath);
  if (!llvmIR)
    return false;

  output << llvmIR.get()->getBuffer();
  return true;
}

enum class EmitAction {
  None,
  Tokens,
  AST,
  MLIR,
  ZCMLIR,
  LoweredMLIR,
  LLVMIR,
  RiscVAssembly,
};

cl::opt<std::string> InputFilename(
    cl::Positional,
    cl::desc("<input .zc file>"),
    cl::init(""));

cl::opt<std::string> OutputFilename(
    "o",
    cl::desc("Write output to <file>"),
    cl::value_desc("file"),
    cl::init(""));

cl::opt<EmitAction> Emit(
    "emit",
    cl::desc("Choose compiler output for the current phase"),
    cl::values(
        clEnumValN(EmitAction::Tokens, "tokens", "Print lexer tokens"),
        clEnumValN(EmitAction::AST, "ast", "Print parsed AST"),
        clEnumValN(EmitAction::MLIR, "mlir", "Print generated standard MLIR"),
        clEnumValN(EmitAction::ZCMLIR, "zc-mlir", "Print generated zc MLIR"),
        clEnumValN(EmitAction::LoweredMLIR, "lowered-mlir",
                   "Print zc MLIR lowered to standard MLIR"),
        clEnumValN(EmitAction::LLVMIR, "llvm", "Print generated LLVM IR"),
        clEnumValN(EmitAction::RiscVAssembly, "riscv-asm",
                   "Print generated RISC-V assembly")),
    cl::init(EmitAction::None));

cl::opt<bool> EmitTokens(
    "emit-tokens",
    cl::desc("Print lexer tokens"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitAST(
    "emit-ast",
    cl::desc("Print parsed AST"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitMLIR(
    "emit-mlir",
    cl::desc("Print generated standard MLIR"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitZCMLIR(
    "emit-zc-mlir",
    cl::desc("Print generated zc dialect MLIR"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitLoweredMLIR(
    "emit-lowered-mlir",
    cl::desc("Print zc dialect lowered to standard MLIR"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitLLVMIR(
    "emit-llvm",
    cl::desc("Print generated LLVM IR"),
    cl::init(false),
    cl::NotHidden);

cl::opt<bool> EmitRiscVAssembly(
    "emit-riscv-asm",
    cl::desc("Print generated RISC-V assembly"),
    cl::init(false),
    cl::NotHidden);

const char *getEmitName(EmitAction action) {
  switch (action) {
  case EmitAction::None:
    return "none";
  case EmitAction::Tokens:
    return "tokens";
  case EmitAction::AST:
    return "ast";
  case EmitAction::MLIR:
    return "mlir";
  case EmitAction::ZCMLIR:
    return "zc-mlir";
  case EmitAction::LoweredMLIR:
    return "lowered-mlir";
  case EmitAction::LLVMIR:
    return "llvm";
  case EmitAction::RiscVAssembly:
    return "riscv-asm";
  }
  return "unknown";
}

} // namespace

int main(int argc, char **argv) {
  InitLLVM initLLVM(argc, argv);

  cl::SetVersionPrinter([](raw_ostream &os) {
    os << "zcompiler phase11 toy compiler\n";
  });

  cl::ParseCommandLineOptions(
      argc, argv,
      "zcompiler toy compiler\n\n"
      "Phase 11 provides lexer, parser, AST dump, MLIR text emission, zc MLIR "
      "text emission, zc-to-standard lowering text, LLVM IR text emission, "
      "and RISC-V assembly text emission.\n");

  if (EmitTokens)
    Emit = EmitAction::Tokens;
  if (EmitAST)
    Emit = EmitAction::AST;
  if (EmitMLIR)
    Emit = EmitAction::MLIR;
  if (EmitZCMLIR)
    Emit = EmitAction::ZCMLIR;
  if (EmitLoweredMLIR)
    Emit = EmitAction::LoweredMLIR;
  if (EmitLLVMIR)
    Emit = EmitAction::LLVMIR;
  if (EmitRiscVAssembly)
    Emit = EmitAction::RiscVAssembly;

  if (InputFilename.empty()) {
    WithColor::error(errs(), "zc") << "missing input .zc file\n";
    errs() << "Try: zc --help\n";
    return 1;
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> inputBuffer =
      MemoryBuffer::getFile(InputFilename);
  if (!inputBuffer) {
    WithColor::error(errs(), "zc")
        << "cannot read input file '" << InputFilename
        << "': " << inputBuffer.getError().message() << "\n";
    return 1;
  }

  std::unique_ptr<raw_fd_ostream> outputFile;
  if (!OutputFilename.empty()) {
    std::error_code ec;
    outputFile = std::make_unique<raw_fd_ostream>(OutputFilename, ec,
                                                  sys::fs::OF_Text);
    if (ec) {
      WithColor::error(errs(), "zc")
          << "cannot open output file '" << OutputFilename
          << "': " << ec.message() << "\n";
      return 1;
    }
  }
  raw_ostream &output = outputFile ? *outputFile : outs();

  if (Emit == EmitAction::None) {
    output << "no emit action selected; use --emit=tokens, --emit=ast, or "
               "--emit=mlir\n";
    return 0;
  }

  zc::Lexer lexer(inputBuffer.get()->getBuffer());
  std::vector<zc::Token> tokens = lexer.lexAll();

  if (Emit == EmitAction::Tokens) {
    zc::printTokens(tokens, output);
    if (lexer.hasError()) {
      for (const std::string &diagnostic : lexer.getDiagnostics())
        WithColor::error(errs(), "zc") << diagnostic << "\n";
      return 1;
    }
    return 0;
  }

  if (lexer.hasError()) {
    for (const std::string &diagnostic : lexer.getDiagnostics())
      WithColor::error(errs(), "zc") << diagnostic << "\n";
    return 1;
  }

  if (Emit == EmitAction::AST) {
    zc::Parser parser(tokens);
    std::unique_ptr<zc::ModuleAST> module = parser.parseModule();
    if (parser.hasError()) {
      for (const std::string &diagnostic : parser.getDiagnostics())
        WithColor::error(errs(), "zc") << diagnostic << "\n";
      return 1;
    }
    module->dump(output);
    return 0;
  }

  zc::Parser parser(tokens);
  std::unique_ptr<zc::ModuleAST> module = parser.parseModule();
  if (parser.hasError()) {
    for (const std::string &diagnostic : parser.getDiagnostics())
      WithColor::error(errs(), "zc") << diagnostic << "\n";
    return 1;
  }

  zc::CodeGenResult codeGenResult;
  switch (Emit) {
  case EmitAction::MLIR:
    {
      mlir::MLIRContext context;
      zc::MLIRGenResult mlirGenResult;
      mlir::OwningOpRef<mlir::ModuleOp> mlirModule =
          zc::generateMLIRModule(context, *module, mlirGenResult);
      if (!mlirGenResult.succeeded()) {
        for (const std::string &diagnostic : mlirGenResult.getDiagnostics())
          WithColor::error(errs(), "zc") << diagnostic << "\n";
        return 1;
      }
      mlirModule->print(output);
      output << "\n";
      return 0;
    }
    break;
  case EmitAction::ZCMLIR:
    codeGenResult = zc::emitZCMLIR(*module, output);
    break;
  case EmitAction::LoweredMLIR:
    codeGenResult = zc::emitLoweredMLIR(*module, output);
    break;
  case EmitAction::LLVMIR:
    if (emitLLVMViaMLIRPipeline(*module, output))
      return 0;
    codeGenResult = zc::emitLLVMIR(*module, output);
    break;
  case EmitAction::RiscVAssembly:
    codeGenResult = zc::emitRiscVAssembly(*module, output);
    break;
  case EmitAction::None:
  case EmitAction::Tokens:
  case EmitAction::AST:
    break;
  }

  if (!codeGenResult.succeeded()) {
    for (const std::string &diagnostic : codeGenResult.getDiagnostics())
      WithColor::error(errs(), "zc") << diagnostic << "\n";
    return 1;
  }

  if (Emit == EmitAction::MLIR || Emit == EmitAction::ZCMLIR ||
      Emit == EmitAction::LoweredMLIR || Emit == EmitAction::LLVMIR ||
      Emit == EmitAction::RiscVAssembly)
    return 0;

  output << "selected action '" << getEmitName(Emit)
         << "' is not implemented until the next compiler phase\n";
  return 0;
}
