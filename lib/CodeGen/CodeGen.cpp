#include "zcompiler/CodeGen/CodeGen.h"

#include "llvm/ADT/StringRef.h"

#include <map>
#include <string>
#include <vector>

using namespace llvm;

namespace zc {

namespace {

enum class TextDialect {
  Standard,
  ZC,
  LLVMIR,
  RiscVAssembly,
};

struct EmittedValue {
  std::string name;
  std::string type;
};

StringRef getLLVMType(StringRef sourceType) {
  if (sourceType == "ptr<i32>")
    return "i32*";
  return "i32";
}

StringRef getMLIRType(StringRef sourceType) {
  if (sourceType == "ptr<i32>")
    return "memref<?xi32>";
  return "i32";
}

class FunctionEmitter {
public:
  FunctionEmitter(raw_ostream &os, CodeGenResult &result, TextDialect dialect)
      : os(os), result(result), dialect(dialect) {}

  void emitFunction(const FunctionAST &function) {
    nextValueID = 0;
    variables.clear();

    if (dialect == TextDialect::LLVMIR) {
      os << "define i32 @" << function.getName() << "(";
      for (size_t index = 0; index < function.getParameters().size();
           ++index) {
        if (index != 0)
          os << ", ";
        const ParameterAST &parameter = function.getParameters()[index];
        os << getLLVMType(parameter.getType()) << " %" << parameter.getName();
      }
      os << ") {\n";
      os << "entry:\n";
      for (const auto &parameter : function.getParameters())
        variables[parameter.getName()] = {"%" + parameter.getName(),
                                          parameter.getType()};
    } else if (dialect == TextDialect::RiscVAssembly) {
      os << "  .text\n";
      os << "  .globl " << function.getName() << "\n";
      os << function.getName() << ":\n";
      for (size_t index = 0; index < function.getParameters().size();
           ++index)
        variables[function.getParameters()[index].getName()] =
            {"a" + std::to_string(index),
             function.getParameters()[index].getType()};
    } else {
      const char *funcOp =
          dialect == TextDialect::ZC ? "zc.func" : "func.func";
      os << "  " << funcOp << " @" << function.getName() << "(";
      for (size_t index = 0; index < function.getParameters().size();
           ++index) {
        if (index != 0)
          os << ", ";
        const ParameterAST &parameter = function.getParameters()[index];
        std::string value = nextSSAValue();
        variables[parameter.getName()] = {value, parameter.getType()};
        os << value << ": " << getMLIRType(parameter.getType());
      }
      os << ") -> " << function.getReturnType() << " {\n";
    }

    for (const auto &statement : function.getBody())
      emitStatement(*statement);

    if (dialect == TextDialect::LLVMIR) {
      if (!blockTerminated)
        os << "  ret i32 0\n";
      os << "}\n";
    } else if (dialect == TextDialect::RiscVAssembly) {
      if (!blockTerminated) {
        os << "  li a0, 0\n";
        os << "  ret\n";
      }
    } else {
      os << "  }\n";
    }
  }

private:
  EmittedValue emitExpression(const ExprAST &expression) {
    switch (expression.getKind()) {
    case ExprKind::Integer:
      return emitInteger(static_cast<const IntegerExprAST &>(expression));
    case ExprKind::Variable:
      return emitVariable(static_cast<const VariableExprAST &>(expression));
    case ExprKind::Binary:
      return emitBinary(static_cast<const BinaryExprAST &>(expression));
    case ExprKind::Call:
      return emitCall(static_cast<const CallExprAST &>(expression));
    case ExprKind::Load:
      return emitLoad(static_cast<const LoadExprAST &>(expression));
    }
    result.addDiagnostic("unknown expression kind");
    return {"", ""};
  }

  EmittedValue emitInteger(const IntegerExprAST &expression) {
    if (dialect == TextDialect::LLVMIR)
      return {expression.getValue(), "i32"};

    if (dialect == TextDialect::RiscVAssembly) {
      std::string reg = nextTempRegister();
      os << "  li " << reg << ", " << expression.getValue() << "\n";
      return {reg, "i32"};
    }

    std::string value = nextSSAValue();
    os << "    " << value << " = ";
    if (dialect == TextDialect::ZC)
      os << "zc.constant ";
    else
      os << "arith.constant ";
    os << expression.getValue() << " : i32\n";
    return {value, "i32"};
  }

  EmittedValue emitVariable(const VariableExprAST &expression) {
    auto found = variables.find(expression.getName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown variable '" + expression.getName() + "'");
      return {"", ""};
    }
    return found->second;
  }

  EmittedValue emitBinary(const BinaryExprAST &expression) {
    EmittedValue lhs = emitExpression(expression.getLHS());
    EmittedValue rhs = emitExpression(expression.getRHS());
    if (lhs.name.empty() || rhs.name.empty())
      return {"", ""};

    std::string value = nextSSAValue();
    std::string resultType = isComparisonOp(expression.getOp()) ? "i1" : "i32";

    if (dialect == TextDialect::LLVMIR) {
      if (isComparisonOp(expression.getOp()))
        os << "  " << value << " = icmp " << getLLVMCmpPredicate(expression.getOp())
           << " i32 " << lhs.name << ", " << rhs.name << "\n";
      else
        os << "  " << value << " = " << getLLVMOpcode(expression.getOp())
           << " i32 " << lhs.name << ", " << rhs.name << "\n";
      return {value, resultType};
    }

    if (dialect == TextDialect::RiscVAssembly) {
      std::string reg = nextTempRegister();
      emitRiscVBinary(expression.getOp(), lhs.name, rhs.name, reg);
      return {reg, resultType};
    }

    os << "    " << value << " = ";
    if (dialect == TextDialect::ZC)
      os << "zc." << getZCOpcode(expression.getOp());
    else
      os << "arith." << getArithOpcode(expression.getOp());
    os << ' ' << lhs.name << ", " << rhs.name << " : i32\n";
    return {value, resultType};
  }

  EmittedValue emitCall(const CallExprAST &expression) {
    std::vector<EmittedValue> args;
    for (const auto &arg : expression.getArgs()) {
      EmittedValue value = emitExpression(*arg);
      if (value.name.empty())
        return {"", ""};
      args.push_back(value);
    }

    std::string value = nextSSAValue();
    if (dialect == TextDialect::LLVMIR) {
      os << "  " << value << " = call i32 @" << expression.getCallee() << "(";
      for (size_t index = 0; index < args.size(); ++index) {
        if (index != 0)
          os << ", ";
        os << getLLVMType(args[index].type) << " " << args[index].name;
      }
      os << ")\n";
      return {value, "i32"};
    }

    if (dialect == TextDialect::RiscVAssembly) {
      for (size_t index = 0; index < args.size() && index < 8; ++index)
        os << "  mv a" << index << ", " << args[index].name << "\n";
      os << "  call " << expression.getCallee() << "\n";
      std::string reg = nextTempRegister();
      os << "  mv " << reg << ", a0\n";
      return {reg, "i32"};
    }

    os << "    " << value << " = ";
    if (dialect == TextDialect::ZC)
      os << "zc.call @" << expression.getCallee() << "(";
    else
      os << "func.call @" << expression.getCallee() << "(";
    for (size_t index = 0; index < args.size(); ++index) {
      if (index != 0)
        os << ", ";
      os << args[index].name;
    }
    os << ") : (";
    for (size_t index = 0; index < args.size(); ++index) {
      if (index != 0)
        os << ", ";
      os << getMLIRType(args[index].type);
    }
    os << ") -> i32\n";
    return {value, "i32"};
  }

  EmittedValue emitLoad(const LoadExprAST &expression) {
    auto found = variables.find(expression.getBufferName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown buffer '" + expression.getBufferName() +
                           "'");
      return {"", ""};
    }

    EmittedValue index = emitExpression(expression.getIndex());
    if (index.name.empty())
      return {"", ""};

    std::string value = nextSSAValue();
    if (dialect == TextDialect::LLVMIR) {
      std::string address = nextSSAValue();
      os << "  " << address << " = getelementptr inbounds i32, i32* "
         << found->second.name << ", i32 " << index.name << "\n";
      os << "  " << value << " = load i32, i32* " << address << ", align 4\n";
      return {value, "i32"};
    }

    if (dialect == TextDialect::RiscVAssembly) {
      std::string offset = nextTempRegister();
      std::string address = nextTempRegister();
      os << "  slli " << offset << ", " << index.name << ", 2\n";
      os << "  add " << address << ", " << found->second.name << ", "
         << offset << "\n";
      std::string reg = nextTempRegister();
      os << "  lw " << reg << ", 0(" << address << ")\n";
      return {reg, "i32"};
    }

    if (dialect == TextDialect::ZC) {
      os << "    " << value << " = zc.load " << found->second.name << "["
         << index.name << "] : i32\n";
      return {value, "i32"};
    }

    std::string indexValue = nextSSAValue();
    os << "    " << indexValue << " = arith.index_cast " << index.name
       << " : i32 to index\n";
    os << "    " << value << " = memref.load " << found->second.name << "["
       << indexValue << "] : memref<?xi32>\n";
    return {value, "i32"};
  }

  void emitStatement(const StmtAST &statement) {
    switch (statement.getKind()) {
    case StmtKind::Let:
      emitLet(static_cast<const LetStmtAST &>(statement));
      return;
    case StmtKind::Assign:
      emitAssign(static_cast<const AssignStmtAST &>(statement));
      return;
    case StmtKind::Store:
      emitStore(static_cast<const StoreStmtAST &>(statement));
      return;
    case StmtKind::Return:
      emitReturn(static_cast<const ReturnStmtAST &>(statement));
      return;
    case StmtKind::If:
      emitIf(static_cast<const IfStmtAST &>(statement));
      return;
    case StmtKind::While:
      emitWhile(static_cast<const WhileStmtAST &>(statement));
      return;
    case StmtKind::VectorAdd:
      emitVectorAdd(static_cast<const VectorAddStmtAST &>(statement));
      return;
    case StmtKind::VectorCopy:
      emitVectorCopy(static_cast<const VectorCopyStmtAST &>(statement));
      return;
    }
    result.addDiagnostic("unknown statement kind");
  }

  void emitLet(const LetStmtAST &statement) {
    EmittedValue value = emitExpression(statement.getValue());
    if (!value.name.empty())
      variables[statement.getName()] = value;
  }

  void emitAssign(const AssignStmtAST &statement) {
    EmittedValue value = emitExpression(statement.getValue());
    if (!value.name.empty())
      variables[statement.getName()] = value;
  }

  void emitStore(const StoreStmtAST &statement) {
    auto found = variables.find(statement.getBufferName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown buffer '" + statement.getBufferName() +
                           "'");
      return;
    }

    EmittedValue index = emitExpression(statement.getIndex());
    EmittedValue value = emitExpression(statement.getValue());
    if (index.name.empty() || value.name.empty())
      return;

    if (dialect == TextDialect::LLVMIR) {
      std::string address = nextSSAValue();
      os << "  " << address << " = getelementptr inbounds i32, i32* "
         << found->second.name << ", i32 " << index.name << "\n";
      os << "  store i32 " << value.name << ", i32* " << address
         << ", align 4\n";
      return;
    }

    if (dialect == TextDialect::RiscVAssembly) {
      std::string offset = nextTempRegister();
      std::string address = nextTempRegister();
      os << "  slli " << offset << ", " << index.name << ", 2\n";
      os << "  add " << address << ", " << found->second.name << ", "
         << offset << "\n";
      os << "  sw " << value.name << ", 0(" << address << ")\n";
      return;
    }

    if (dialect == TextDialect::ZC) {
      os << "    zc.store " << value.name << ", " << found->second.name
         << "[" << index.name << "] : i32\n";
      return;
    }

    std::string indexValue = nextSSAValue();
    os << "    " << indexValue << " = arith.index_cast " << index.name
       << " : i32 to index\n";
    os << "    memref.store " << value.name << ", " << found->second.name
       << "[" << indexValue << "] : memref<?xi32>\n";
  }

  void emitVectorAdd(const VectorAddStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_add text lowering is only available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_add statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string lhsAddress = nextTempRegister();
    std::string rhsAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_add");
    std::string endLabel = nextLabel(".Lvector_add_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel
       << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    os << "  add " << lhsAddress << ", " << lhs->second.name << ", "
       << offset << "\n";
    os << "  add " << rhsAddress << ", " << rhs->second.name << ", "
       << offset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle32.v v0, 0(" << lhsAddress << ")\n";
    os << "  vle32.v v1, 0(" << rhsAddress << ")\n";
    os << "  vadd.vv v2, v0, v1\n";
    os << "  vse32.v v2, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorCopy(const VectorCopyStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_copy text lowering is only available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_copy statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string inputAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_copy");
    std::string endLabel = nextLabel(".Lvector_copy_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel
       << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    os << "  add " << inputAddress << ", " << input->second.name << ", "
       << offset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle32.v v0, 0(" << inputAddress << ")\n";
    os << "  vse32.v v0, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitReturn(const ReturnStmtAST &statement) {
    EmittedValue value = emitExpression(statement.getValue());
    if (value.name.empty())
      return;

    if (dialect == TextDialect::LLVMIR) {
      EmittedValue i32Value = ensureI32(value);
      os << "  ret i32 " << i32Value.name << "\n";
      blockTerminated = true;
    } else if (dialect == TextDialect::RiscVAssembly) {
      os << "  mv a0, " << value.name << "\n";
      os << "  ret\n";
      blockTerminated = true;
    }
    else if (dialect == TextDialect::ZC)
      os << "    zc.return " << value.name << " : i32\n";
    else
      os << "    return " << value.name << " : i32\n";
  }

  void emitIf(const IfStmtAST &statement) {
    if (dialect == TextDialect::LLVMIR) {
      EmittedValue condition = ensureI1(emitExpression(statement.getCondition()));
      std::string thenLabel = nextLabel("if.then");
      std::string elseLabel = nextLabel("if.else");
      std::string endLabel = nextLabel("if.end");

      os << "  br i1 " << condition.name << ", label %" << thenLabel
         << ", label %" << elseLabel << "\n";
      os << thenLabel << ":\n";
      blockTerminated = false;
      for (const auto &bodyStatement : statement.getThenBody())
        emitStatement(*bodyStatement);
      if (!blockTerminated)
        os << "  br label %" << endLabel << "\n";

      os << elseLabel << ":\n";
      blockTerminated = false;
      for (const auto &bodyStatement : statement.getElseBody())
        emitStatement(*bodyStatement);
      if (!blockTerminated)
        os << "  br label %" << endLabel << "\n";

      os << endLabel << ":\n";
      blockTerminated = false;
      return;
    }

    if (dialect == TextDialect::RiscVAssembly) {
      EmittedValue condition = emitExpression(statement.getCondition());
      std::string elseLabel = nextLabel(".Lelse");
      std::string endLabel = nextLabel(".Lendif");
      os << "  beqz " << condition.name << ", " << elseLabel << "\n";
      for (const auto &bodyStatement : statement.getThenBody())
        emitStatement(*bodyStatement);
      os << "  j " << endLabel << "\n";
      os << elseLabel << ":\n";
      for (const auto &bodyStatement : statement.getElseBody())
        emitStatement(*bodyStatement);
      os << endLabel << ":\n";
      return;
    }

    EmittedValue condition = emitExpression(statement.getCondition());
    os << "    " << (dialect == TextDialect::ZC ? "zc.if " : "scf.if ")
       << condition.name << " {\n";
    for (const auto &bodyStatement : statement.getThenBody())
      emitStatement(*bodyStatement);
    if (!statement.getElseBody().empty()) {
      os << "    } else {\n";
      for (const auto &bodyStatement : statement.getElseBody())
        emitStatement(*bodyStatement);
    }
    os << "    }\n";
  }

  void emitWhile(const WhileStmtAST &statement) {
    if (dialect == TextDialect::LLVMIR) {
      std::string condLabel = nextLabel("while.cond");
      std::string bodyLabel = nextLabel("while.body");
      std::string endLabel = nextLabel("while.end");
      os << "  br label %" << condLabel << "\n";
      os << condLabel << ":\n";
      blockTerminated = false;
      EmittedValue condition = ensureI1(emitExpression(statement.getCondition()));
      os << "  br i1 " << condition.name << ", label %" << bodyLabel
         << ", label %" << endLabel << "\n";
      os << bodyLabel << ":\n";
      blockTerminated = false;
      for (const auto &bodyStatement : statement.getBody())
        emitStatement(*bodyStatement);
      if (!blockTerminated)
        os << "  br label %" << condLabel << "\n";
      os << endLabel << ":\n";
      blockTerminated = false;
      return;
    }

    if (dialect == TextDialect::RiscVAssembly) {
      std::string condLabel = nextLabel(".Lwhile");
      std::string endLabel = nextLabel(".Lendwhile");
      os << condLabel << ":\n";
      EmittedValue condition = emitExpression(statement.getCondition());
      os << "  beqz " << condition.name << ", " << endLabel << "\n";
      for (const auto &bodyStatement : statement.getBody())
        emitStatement(*bodyStatement);
      os << "  j " << condLabel << "\n";
      os << endLabel << ":\n";
      return;
    }

    EmittedValue condition = emitExpression(statement.getCondition());
    os << "    " << (dialect == TextDialect::ZC ? "zc.while " : "scf.while ")
       << condition.name << " {\n";
    for (const auto &bodyStatement : statement.getBody())
      emitStatement(*bodyStatement);
    os << "    }\n";
  }

  std::string nextSSAValue() {
    std::string value = "%" + std::to_string(nextValueID);
    ++nextValueID;
    return value;
  }

  std::string nextTempRegister() {
    std::string reg = "t" + std::to_string(nextRegisterID % 7);
    ++nextRegisterID;
    return reg;
  }

  std::string nextLabel(StringRef prefix) {
    std::string label = prefix.str() + "." + std::to_string(nextLabelID);
    ++nextLabelID;
    return label;
  }

  EmittedValue ensureI1(EmittedValue value) {
    if (value.type == "i1")
      return value;
    std::string compare = nextSSAValue();
    os << "  " << compare << " = icmp ne i32 " << value.name << ", 0\n";
    return {compare, "i1"};
  }

  EmittedValue ensureI32(EmittedValue value) {
    if (value.type == "i32")
      return value;
    std::string extended = nextSSAValue();
    os << "  " << extended << " = zext i1 " << value.name << " to i32\n";
    return {extended, "i32"};
  }

  bool isComparisonOp(StringRef op) const {
    return op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" ||
           op == "!=";
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
    if (op == "<")
      return "cmpi slt,";
    if (op == "<=")
      return "cmpi sle,";
    if (op == ">")
      return "cmpi sgt,";
    if (op == ">=")
      return "cmpi sge,";
    if (op == "==")
      return "cmpi eq,";
    if (op == "!=")
      return "cmpi ne,";
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
    if (op == "<")
      return "cmp_lt";
    if (op == "<=")
      return "cmp_le";
    if (op == ">")
      return "cmp_gt";
    if (op == ">=")
      return "cmp_ge";
    if (op == "==")
      return "cmp_eq";
    if (op == "!=")
      return "cmp_ne";
    result.addDiagnostic("unsupported binary operator '" + op.str() + "'");
    return "unknown";
  }

  StringRef getLLVMCmpPredicate(StringRef op) {
    if (op == "<")
      return "slt";
    if (op == "<=")
      return "sle";
    if (op == ">")
      return "sgt";
    if (op == ">=")
      return "sge";
    if (op == "==")
      return "eq";
    if (op == "!=")
      return "ne";
    result.addDiagnostic("unsupported comparison operator '" + op.str() + "'");
    return "eq";
  }

  void emitRiscVBinary(StringRef op, StringRef lhs, StringRef rhs,
                       StringRef dest) {
    if (op == "+")
      os << "  add " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "-")
      os << "  sub " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "*")
      os << "  mul " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "/")
      os << "  div " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "<")
      os << "  slt " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == ">")
      os << "  slt " << dest << ", " << rhs << ", " << lhs << "\n";
    else if (op == "<=") {
      os << "  slt " << dest << ", " << rhs << ", " << lhs << "\n";
      os << "  xori " << dest << ", " << dest << ", 1\n";
    } else if (op == ">=") {
      os << "  slt " << dest << ", " << lhs << ", " << rhs << "\n";
      os << "  xori " << dest << ", " << dest << ", 1\n";
    } else if (op == "==") {
      os << "  sub " << dest << ", " << lhs << ", " << rhs << "\n";
      os << "  seqz " << dest << ", " << dest << "\n";
    } else if (op == "!=") {
      os << "  sub " << dest << ", " << lhs << ", " << rhs << "\n";
      os << "  snez " << dest << ", " << dest << "\n";
    } else {
      result.addDiagnostic("unsupported RISC-V operator '" + op.str() + "'");
    }
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
  unsigned nextRegisterID = 0;
  unsigned nextLabelID = 0;
  bool blockTerminated = false;
  std::map<std::string, EmittedValue> variables;
};

CodeGenResult emitModule(const ModuleAST &module, raw_ostream &os,
                         TextDialect dialect) {
  CodeGenResult result;

  if (dialect == TextDialect::LLVMIR) {
    os << "; ModuleID = 'zcompiler'\n";
    os << "source_filename = \"zcompiler\"\n\n";
  } else if (dialect == TextDialect::RiscVAssembly) {
    os << "  .option nopic\n";
  } else {
    os << "module {\n";
  }

  for (const auto &function : module.getFunctions()) {
    FunctionEmitter emitter(os, result, dialect);
    emitter.emitFunction(*function);
    if (dialect == TextDialect::LLVMIR)
      os << '\n';
  }

  if (dialect != TextDialect::LLVMIR && dialect != TextDialect::RiscVAssembly)
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

CodeGenResult emitRiscVAssembly(const ModuleAST &module, raw_ostream &os) {
  return emitModule(module, os, TextDialect::RiscVAssembly);
}

} // namespace zc
