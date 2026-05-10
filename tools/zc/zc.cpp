#include "zcompiler/Lexer/Lexer.h"

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
        clEnumValN(EmitAction::MLIR, "mlir", "Print generated MLIR")),
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
    cl::desc("Print generated MLIR"),
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
  }
  return "unknown";
}

} // namespace

int main(int argc, char **argv) {
  InitLLVM initLLVM(argc, argv);

  cl::SetVersionPrinter([](raw_ostream &os) {
    os << "zcompiler phase2 toy compiler lexer\n";
  });

  cl::ParseCommandLineOptions(
      argc, argv,
      "zcompiler toy compiler\n\n"
      "Phase 2 provides the command-line driver and lexer. Parser and MLIR "
      "emission are implemented in later phases.\n");

  if (EmitTokens)
    Emit = EmitAction::Tokens;
  if (EmitAST)
    Emit = EmitAction::AST;
  if (EmitMLIR)
    Emit = EmitAction::MLIR;

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

  if (Emit == EmitAction::Tokens) {
    zc::Lexer lexer(inputBuffer.get()->getBuffer());
    std::vector<zc::Token> tokens = lexer.lexAll();
    zc::printTokens(tokens, output);
    if (lexer.hasError()) {
      for (const std::string &diagnostic : lexer.getDiagnostics())
        WithColor::error(errs(), "zc") << diagnostic << "\n";
      return 1;
    }
    return 0;
  }

  output << "selected action '" << getEmitName(Emit)
         << "' is not implemented until the next compiler phase\n";
  return 0;
}
