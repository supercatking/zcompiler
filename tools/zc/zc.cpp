#include "zcompiler/CodeGen/CodeGen.h"
#include "zcompiler/Lexer/Lexer.h"
#include "zcompiler/Parser/Parser.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>

using namespace llvm;

namespace {

enum class EmitAction {
  None,
  Tokens,
  AST,
  MLIR,
  ZCMLIR,
  LoweredMLIR,
  LLVMIR,
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
        clEnumValN(EmitAction::LLVMIR, "llvm", "Print generated LLVM IR")),
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
  }
  return "unknown";
}

} // namespace

int main(int argc, char **argv) {
  InitLLVM initLLVM(argc, argv);

  cl::SetVersionPrinter([](raw_ostream &os) {
    os << "zcompiler phase7 toy compiler llvm-ir\n";
  });

  cl::ParseCommandLineOptions(
      argc, argv,
      "zcompiler toy compiler\n\n"
      "Phase 7 provides lexer, parser, AST dump, MLIR text emission, zc MLIR "
      "text emission, zc-to-standard lowering text, and LLVM IR text emission.\n");

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
    codeGenResult = zc::emitStandardMLIR(*module, output);
    break;
  case EmitAction::ZCMLIR:
    codeGenResult = zc::emitZCMLIR(*module, output);
    break;
  case EmitAction::LoweredMLIR:
    codeGenResult = zc::emitLoweredMLIR(*module, output);
    break;
  case EmitAction::LLVMIR:
    codeGenResult = zc::emitLLVMIR(*module, output);
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
      Emit == EmitAction::LoweredMLIR || Emit == EmitAction::LLVMIR)
    return 0;

  output << "selected action '" << getEmitName(Emit)
         << "' is not implemented until the next compiler phase\n";
  return 0;
}
