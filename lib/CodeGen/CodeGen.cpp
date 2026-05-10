#include "zcompiler/CodeGen/CodeGen.h"

#include "llvm/ADT/StringRef.h"

#include <map>
#include <string>

using namespace llvm;

namespace zc {

namespace {

enum class TextDialect {
  Standard,
  ZC,
  LLVMIR,
};

class FunctionEmitter {
public:
  FunctionEmitter(raw_ostream &os, CodeGenResult &result, TextDialect dialect)
      : os(os), result(result), dialect(dialect) {}

  void emitFunction(const FunctionAST &function) {
    nextValueID = 0;
    variables.clear();

    if (dialect == TextDialect::LLVMIR) {
      os << "define i32 @" << function.getName() << "() {\n";
      os << "entry:\n";
    } else {
      const char *funcOp =
          dialect == TextDialect::ZC ? "zc.func" : "func.func";
      os << "  " << funcOp << " @" << function.getName() << "() -> "
         << function.getReturnType() << " {\n";
    }

    for (const auto &statement : function.getBody())
      emitStatement(*statement);

    if (dialect == TextDialect::LLVMIR)
      os << "}\n";
    else
      os << "  }\n";
  }

private:
  std::string emitExpression(const ExprAST &expression) {
    switch (expression.getKind()) {
    case ExprKind::Integer:
      return emitInteger(static_cast<const IntegerExprAST &>(expression));
    case ExprKind::Variable:
      return emitVariable(static_cast<const VariableExprAST &>(expression));
    case ExprKind::Binary:
      return emitBinary(static_cast<const BinaryExprAST &>(expression));
    }
    result.addDiagnostic("unknown expression kind");
    return "";
  }

  std::string emitInteger(const IntegerExprAST &expression) {
    if (dialect == TextDialect::LLVMIR)
      return expression.getValue();

    std::string value = nextSSAValue();
    os << "    " << value << " = ";
    if (dialect == TextDialect::ZC)
      os << "zc.constant ";
    else
      os << "arith.constant ";
    os << expression.getValue() << " : i32\n";
    return value;
  }

  std::string emitVariable(const VariableExprAST &expression) {
    auto found = variables.find(expression.getName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown variable '" + expression.getName() + "'");
      return "";
    }
    return found->second;
  }

  std::string emitBinary(const BinaryExprAST &expression) {
    std::string lhs = emitExpression(expression.getLHS());
    std::string rhs = emitExpression(expression.getRHS());
    if (lhs.empty() || rhs.empty())
      return "";

    std::string value = nextSSAValue();
    if (dialect == TextDialect::LLVMIR) {
      os << "  " << value << " = " << getLLVMOpcode(expression.getOp())
         << " i32 " << lhs << ", " << rhs << "\n";
      return value;
    }

    os << "    " << value << " = ";
    if (dialect == TextDialect::ZC)
      os << "zc." << getZCOpcode(expression.getOp());
    else
      os << "arith." << getArithOpcode(expression.getOp());
    os << ' ' << lhs << ", " << rhs << " : i32\n";
    return value;
  }

  void emitStatement(const StmtAST &statement) {
    switch (statement.getKind()) {
    case StmtKind::Let:
      emitLet(static_cast<const LetStmtAST &>(statement));
      return;
    case StmtKind::Return:
      emitReturn(static_cast<const ReturnStmtAST &>(statement));
      return;
    }
    result.addDiagnostic("unknown statement kind");
  }

  void emitLet(const LetStmtAST &statement) {
    std::string value = emitExpression(statement.getValue());
    if (!value.empty())
      variables[statement.getName()] = value;
  }

  void emitReturn(const ReturnStmtAST &statement) {
    std::string value = emitExpression(statement.getValue());
    if (value.empty())
      return;

    if (dialect == TextDialect::LLVMIR)
      os << "  ret i32 " << value << "\n";
    else if (dialect == TextDialect::ZC)
      os << "    zc.return " << value << " : i32\n";
    else
      os << "    return " << value << " : i32\n";
  }

  std::string nextSSAValue() {
    std::string value = "%" + std::to_string(nextValueID);
    ++nextValueID;
    return value;
  }

  StringRef getArithOpcode(StringRef op) {
    if (op == "+")
      return "addi";
    if (op == "-")
      return "subi";
    if (op == "*")
      return "muli";
    if (op == "/")
      return "divsi";
    result.addDiagnostic("unsupported binary operator '" + op.str() + "'");
    return "unknown";
  }

  StringRef getZCOpcode(StringRef op) {
    if (op == "+")
      return "add";
    if (op == "-")
      return "sub";
    if (op == "*")
      return "mul";
    if (op == "/")
      return "div";
    result.addDiagnostic("unsupported binary operator '" + op.str() + "'");
    return "unknown";
  }

  StringRef getLLVMOpcode(StringRef op) {
    if (op == "+")
      return "add";
    if (op == "-")
      return "sub";
    if (op == "*")
      return "mul";
    if (op == "/")
      return "sdiv";
    result.addDiagnostic("unsupported binary operator '" + op.str() + "'");
    return "unknown";
  }

  raw_ostream &os;
  CodeGenResult &result;
  TextDialect dialect;
  unsigned nextValueID = 0;
  std::map<std::string, std::string> variables;
};

CodeGenResult emitModule(const ModuleAST &module, raw_ostream &os,
                         TextDialect dialect) {
  CodeGenResult result;

  if (dialect == TextDialect::LLVMIR) {
    os << "; ModuleID = 'zcompiler'\n";
    os << "source_filename = \"zcompiler\"\n\n";
  } else {
    os << "module {\n";
  }

  for (const auto &function : module.getFunctions()) {
    FunctionEmitter emitter(os, result, dialect);
    emitter.emitFunction(*function);
    if (dialect == TextDialect::LLVMIR)
      os << '\n';
  }

  if (dialect != TextDialect::LLVMIR)
    os << "}\n";

  return result;
}

} // namespace

CodeGenResult emitStandardMLIR(const ModuleAST &module, raw_ostream &os) {
  return emitModule(module, os, TextDialect::Standard);
}

CodeGenResult emitZCMLIR(const ModuleAST &module, raw_ostream &os) {
  return emitModule(module, os, TextDialect::ZC);
}

CodeGenResult emitLoweredMLIR(const ModuleAST &module, raw_ostream &os) {
  return emitStandardMLIR(module, os);
}

CodeGenResult emitLLVMIR(const ModuleAST &module, raw_ostream &os) {
  return emitModule(module, os, TextDialect::LLVMIR);
}

} // namespace zc
