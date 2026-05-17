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

unsigned getIntegerTypeWidth(StringRef sourceType) {
  if (sourceType == "i8" || sourceType == "ptr<i8>")
    return 8;
  if (sourceType == "i16" || sourceType == "ptr<i16>")
    return 16;
  if (sourceType == "i64" || sourceType == "ptr<i64>")
    return 64;
  return 32;
}

StringRef getLLVMType(StringRef sourceType) {
  if (sourceType == "ptr<i8>")
    return "i8*";
  if (sourceType == "ptr<i16>")
    return "i16*";
  if (sourceType == "ptr<i32>")
    return "i32*";
  if (sourceType == "ptr<i64>")
    return "i64*";
  if (sourceType == "i8")
    return "i8";
  if (sourceType == "i16")
    return "i16";
  if (sourceType == "i64")
    return "i64";
  return "i32";
}

StringRef getMLIRType(StringRef sourceType) {
  if (sourceType == "ptr<i8>")
    return "memref<?xi8>";
  if (sourceType == "ptr<i16>")
    return "memref<?xi16>";
  if (sourceType == "ptr<i32>")
    return "memref<?xi32>";
  if (sourceType == "ptr<i64>")
    return "memref<?xi64>";
  if (sourceType == "i8")
    return "i8";
  if (sourceType == "i16")
    return "i16";
  if (sourceType == "i64")
    return "i64";
  return "i32";
}

unsigned getByteShiftForWidth(unsigned width) {
  if (width == 16)
    return 1;
  if (width == 32)
    return 2;
  if (width == 64)
    return 3;
  return 0;
}

StringRef getVectorLMULAssemblyName(VectorLMUL lmul) {
  switch (lmul) {
  case VectorLMUL::M1:
    return "m1";
  case VectorLMUL::M2:
    return "m2";
  case VectorLMUL::M4:
    return "m4";
  }
  return "m1";
}

class FunctionEmitter {
public:
  FunctionEmitter(raw_ostream &os, CodeGenResult &result, TextDialect dialect)
      : os(os), result(result), dialect(dialect) {}

  void emitFunction(const FunctionAST &function) {
    nextValueID = 0;
    variables.clear();
    masks.clear();
    logicalMasks.clear();

    if (dialect == TextDialect::LLVMIR) {
      os << "define i32 @" << function.getName() << "(";
      for (size_t index = 0; index < function.getParameters().size(); ++index) {
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
      for (size_t index = 0; index < function.getParameters().size(); ++index)
        variables[function.getParameters()[index].getName()] = {
            "a" + std::to_string(index),
            function.getParameters()[index].getType()};
    } else {
      const char *funcOp = dialect == TextDialect::ZC ? "zc.func" : "func.func";
      os << "  " << funcOp << " @" << function.getName() << "(";
      for (size_t index = 0; index < function.getParameters().size(); ++index) {
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
        os << "  " << value << " = icmp "
           << getLLVMCmpPredicate(expression.getOp()) << " i32 " << lhs.name
           << ", " << rhs.name << "\n";
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
      os << "  add " << address << ", " << found->second.name << ", " << offset
         << "\n";
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
    case StmtKind::PrintI32:
      emitPrintI32(static_cast<const PrintI32StmtAST &>(statement));
      return;
    case StmtKind::MatrixPackB:
      emitMatrixPackB(static_cast<const MatrixPackBStmtAST &>(statement));
      return;
    case StmtKind::MatrixMultiply:
      emitMatrixMultiply(static_cast<const MatrixMultiplyStmtAST &>(statement));
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
    case StmtKind::VectorStridedLoad:
      emitVectorStridedLoad(
          static_cast<const VectorStridedLoadStmtAST &>(statement));
      return;
    case StmtKind::VectorIndexedLoad:
      emitVectorIndexedLoad(
          static_cast<const VectorIndexedLoadStmtAST &>(statement));
      return;
    case StmtKind::VectorStridedStore:
      emitVectorStridedStore(
          static_cast<const VectorStridedStoreStmtAST &>(statement));
      return;
    case StmtKind::VectorIndexedStore:
      emitVectorIndexedStore(
          static_cast<const VectorIndexedStoreStmtAST &>(statement));
      return;
    case StmtKind::VectorCopy:
      emitVectorCopy(static_cast<const VectorCopyStmtAST &>(statement));
      return;
    case StmtKind::VectorScale:
      emitVectorScale(static_cast<const VectorScaleStmtAST &>(statement));
      return;
    case StmtKind::VectorMul:
      emitVectorMul(static_cast<const VectorMulStmtAST &>(statement));
      return;
    case StmtKind::VectorWidenAdd:
      emitVectorWidenAdd(static_cast<const VectorWidenAddStmtAST &>(statement));
      return;
    case StmtKind::VectorReduceAdd:
      emitVectorReduceAdd(
          static_cast<const VectorReduceAddStmtAST &>(statement));
      return;
    case StmtKind::VectorSelect:
      emitVectorSelect(static_cast<const VectorSelectStmtAST &>(statement));
      return;
    case StmtKind::VectorMask:
      rememberVectorMask(static_cast<const VectorMaskStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskLogical:
      rememberVectorMaskLogical(
          static_cast<const VectorMaskLogicalStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedBinary:
      emitVectorMaskedBinary(
          static_cast<const VectorMaskedBinaryStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedStore:
      emitVectorMaskedStore(
          static_cast<const VectorMaskedStoreStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedLoad:
      emitVectorMaskedLoad(
          static_cast<const VectorMaskedLoadStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedStridedLoad:
      emitVectorMaskedStridedLoad(
          static_cast<const VectorMaskedStridedLoadStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedIndexedLoad:
      emitVectorMaskedIndexedLoad(
          static_cast<const VectorMaskedIndexedLoadStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedStridedStore:
      emitVectorMaskedStridedStore(
          static_cast<const VectorMaskedStridedStoreStmtAST &>(statement));
      return;
    case StmtKind::VectorMaskedIndexedStore:
      emitVectorMaskedIndexedStore(
          static_cast<const VectorMaskedIndexedStoreStmtAST &>(statement));
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
      os << "  add " << address << ", " << found->second.name << ", " << offset
         << "\n";
      os << "  sw " << value.name << ", 0(" << address << ")\n";
      return;
    }

    if (dialect == TextDialect::ZC) {
      os << "    zc.store " << value.name << ", " << found->second.name << "["
         << index.name << "] : i32\n";
      return;
    }

    std::string indexValue = nextSSAValue();
    os << "    " << indexValue << " = arith.index_cast " << index.name
       << " : i32 to index\n";
    os << "    memref.store " << value.name << ", " << found->second.name << "["
       << indexValue << "] : memref<?xi32>\n";
  }

  void emitPrintI32(const PrintI32StmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "print_i32 text lowering is only available for RISC-V assembly");
      return;
    }

    EmittedValue value = emitExpression(statement.getValue());
    if (value.name.empty())
      return;

    emitPrintI32CallWithRegisterSave(value.name);
  }

  void emitMatrixPackB(const MatrixPackBStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("matrix_pack_b text lowering is only available "
                           "for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in matrix_pack_b statement");
      return;
    }

    EmittedValue cols = emitExpression(statement.getCols());
    EmittedValue inner = emitExpression(statement.getInner());
    if (cols.name.empty() || inner.name.empty())
      return;

    std::string colLabel = nextLabel(".Lmatrix_pack_b_col");
    std::string innerLabel = nextLabel(".Lmatrix_pack_b_inner");
    std::string nextColLabel = nextLabel(".Lmatrix_pack_b_next_col");
    std::string endLabel = nextLabel(".Lmatrix_pack_b_end");

    os << "  addi sp, sp, -48\n";
    os << "  sd s0, 0(sp)\n";
    os << "  sd s1, 8(sp)\n";
    os << "  sd s2, 16(sp)\n";
    os << "  sd s3, 24(sp)\n";
    os << "  sd s4, 32(sp)\n";
    os << "  sd s5, 40(sp)\n";
    os << "  mv s0, " << output->second.name << "\n";
    os << "  mv s1, " << input->second.name << "\n";
    os << "  mv s2, " << cols.name << "\n";
    os << "  mv s3, " << inner.name << "\n";
    os << "  li s4, 0\n";
    os << colLabel << ":\n";
    os << "  bgeu s4, s2, " << endLabel << "\n";
    os << "  li s5, 0\n";
    os << innerLabel << ":\n";
    os << "  bgeu s5, s3, " << nextColLabel << "\n";
    os << "  mul t0, s5, s2\n";
    os << "  add t0, t0, s4\n";
    os << "  slli t0, t0, 2\n";
    os << "  add t0, s1, t0\n";
    os << "  lw t1, 0(t0)\n";
    os << "  mul t2, s4, s3\n";
    os << "  add t2, t2, s5\n";
    os << "  slli t2, t2, 2\n";
    os << "  add t2, s0, t2\n";
    os << "  sw t1, 0(t2)\n";
    os << "  addi s5, s5, 1\n";
    os << "  j " << innerLabel << "\n";
    os << nextColLabel << ":\n";
    os << "  addi s4, s4, 1\n";
    os << "  j " << colLabel << "\n";
    os << endLabel << ":\n";
    os << "  ld s0, 0(sp)\n";
    os << "  ld s1, 8(sp)\n";
    os << "  ld s2, 16(sp)\n";
    os << "  ld s3, 24(sp)\n";
    os << "  ld s4, 32(sp)\n";
    os << "  ld s5, 40(sp)\n";
    os << "  addi sp, sp, 48\n";
  }

  void emitMatrixMultiply(const MatrixMultiplyStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("matrix_multiply text lowering is only available "
                           "for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic("unknown buffer in matrix_multiply statement");
      return;
    }

    EmittedValue rows = emitExpression(statement.getRows());
    EmittedValue cols = emitExpression(statement.getCols());
    EmittedValue inner = emitExpression(statement.getInner());
    if (rows.name.empty() || cols.name.empty() || inner.name.empty())
      return;

    if (statement.getRHSLayout() == MatrixRHSLayout::PackedColumns) {
      std::string rowLabel = nextLabel(".Lmatrix_multiply_packed_b_row");
      std::string colLabel = nextLabel(".Lmatrix_multiply_packed_b_col");
      std::string innerLabel = nextLabel(".Lmatrix_multiply_packed_b_inner");
      std::string storeLabel = nextLabel(".Lmatrix_multiply_packed_b_store");
      std::string nextRowLabel =
          nextLabel(".Lmatrix_multiply_packed_b_next_row");
      std::string endLabel = nextLabel(".Lmatrix_multiply_packed_b_end");

      os << "  addi sp, sp, -96\n";
      os << "  sd s0, 0(sp)\n";
      os << "  sd s1, 8(sp)\n";
      os << "  sd s2, 16(sp)\n";
      os << "  sd s3, 24(sp)\n";
      os << "  sd s4, 32(sp)\n";
      os << "  sd s5, 40(sp)\n";
      os << "  sd s6, 48(sp)\n";
      os << "  sd s7, 56(sp)\n";
      os << "  sd s8, 64(sp)\n";
      os << "  sd s9, 72(sp)\n";
      os << "  sd s10, 80(sp)\n";
      os << "  sd s11, 88(sp)\n";
      os << "  mv s0, " << output->second.name << "\n";
      os << "  mv s1, " << lhs->second.name << "\n";
      os << "  mv s2, " << rhs->second.name << "\n";
      os << "  mv s3, " << rows.name << "\n";
      os << "  mv s4, " << cols.name << "\n";
      os << "  mv s5, " << inner.name << "\n";
      os << "  li s6, 0\n";
      os << rowLabel << ":\n";
      os << "  bgeu s6, s3, " << endLabel << "\n";
      os << "  li s7, 0\n";
      os << colLabel << ":\n";
      os << "  bgeu s7, s4, " << nextRowLabel << "\n";
      os << "  mul s10, s6, s5\n";
      os << "  slli s10, s10, 2\n";
      os << "  add s10, s1, s10\n";
      os << "  mul s11, s7, s5\n";
      os << "  slli s11, s11, 2\n";
      os << "  add s11, s2, s11\n";
      os << "  li s8, 0\n";
      os << "  li s9, 0\n";
      os << innerLabel << ":\n";
      os << "  bgeu s8, s5, " << storeLabel << "\n";
      os << "  sub t0, s5, s8\n";
      os << "  vsetvli t0, t0, e32, m1, ta, ma\n";
      os << "  slli t1, s8, 2\n";
      os << "  add t2, s10, t1\n";
      os << "  add t3, s11, t1\n";
      os << "  vle32.v v0, 0(t2)\n";
      os << "  vle32.v v1, 0(t3)\n";
      os << "  vmul.vv v2, v0, v1\n";
      os << "  vmv.s.x v3, s9\n";
      os << "  vredsum.vs v4, v2, v3\n";
      os << "  vmv.x.s s9, v4\n";
      os << "  add s8, s8, t0\n";
      os << "  j " << innerLabel << "\n";
      os << storeLabel << ":\n";
      os << "  mul t0, s6, s4\n";
      os << "  add t0, t0, s7\n";
      os << "  slli t0, t0, 2\n";
      os << "  add t0, s0, t0\n";
      os << "  sw s9, 0(t0)\n";
      os << "  addi s7, s7, 1\n";
      os << "  j " << colLabel << "\n";
      os << nextRowLabel << ":\n";
      os << "  addi s6, s6, 1\n";
      os << "  j " << rowLabel << "\n";
      os << endLabel << ":\n";
      os << "  ld s0, 0(sp)\n";
      os << "  ld s1, 8(sp)\n";
      os << "  ld s2, 16(sp)\n";
      os << "  ld s3, 24(sp)\n";
      os << "  ld s4, 32(sp)\n";
      os << "  ld s5, 40(sp)\n";
      os << "  ld s6, 48(sp)\n";
      os << "  ld s7, 56(sp)\n";
      os << "  ld s8, 64(sp)\n";
      os << "  ld s9, 72(sp)\n";
      os << "  ld s10, 80(sp)\n";
      os << "  ld s11, 88(sp)\n";
      os << "  addi sp, sp, 96\n";
      return;
    }

    std::string rowLabel = nextLabel(".Lmatrix_multiply_row");
    std::string colLabel = nextLabel(".Lmatrix_multiply_col");
    std::string innerLabel = nextLabel(".Lmatrix_multiply_inner");
    std::string storeLabel = nextLabel(".Lmatrix_multiply_store");
    std::string nextRowLabel = nextLabel(".Lmatrix_multiply_next_row");
    std::string endLabel = nextLabel(".Lmatrix_multiply_end");

    os << "  addi sp, sp, -96\n";
    os << "  sd s0, 0(sp)\n";
    os << "  sd s1, 8(sp)\n";
    os << "  sd s2, 16(sp)\n";
    os << "  sd s3, 24(sp)\n";
    os << "  sd s4, 32(sp)\n";
    os << "  sd s5, 40(sp)\n";
    os << "  sd s6, 48(sp)\n";
    os << "  sd s7, 56(sp)\n";
    os << "  sd s8, 64(sp)\n";
    os << "  sd s9, 72(sp)\n";
    os << "  sd s10, 80(sp)\n";
    os << "  sd s11, 88(sp)\n";
    os << "  mv s0, " << output->second.name << "\n";
    os << "  mv s1, " << lhs->second.name << "\n";
    os << "  mv s2, " << rhs->second.name << "\n";

    os << "  mv s3, " << rows.name << "\n";
    os << "  mv s4, " << cols.name << "\n";
    os << "  mv s5, " << inner.name << "\n";
    os << "  li s6, 0\n";
    os << rowLabel << ":\n";
    os << "  bgeu s6, s3, " << endLabel << "\n";
    os << "  li s7, 0\n";
    os << colLabel << ":\n";
    os << "  bgeu s7, s4, " << nextRowLabel << "\n";
    os << "  li s8, 0\n";
    os << "  li s9, 0\n";
    os << innerLabel << ":\n";
    os << "  bgeu s8, s5, " << storeLabel << "\n";
    os << "  mul s10, s6, s5\n";
    os << "  add s10, s10, s8\n";
    os << "  slli s10, s10, 2\n";
    os << "  add s10, s1, s10\n";
    os << "  lw t0, 0(s10)\n";
    os << "  mul s11, s8, s4\n";
    os << "  add s11, s11, s7\n";
    os << "  slli s11, s11, 2\n";
    os << "  add s11, s2, s11\n";
    os << "  lw t1, 0(s11)\n";
    os << "  mulw t2, t0, t1\n";
    os << "  addw s9, s9, t2\n";
    os << "  addi s8, s8, 1\n";
    os << "  j " << innerLabel << "\n";
    os << storeLabel << ":\n";
    os << "  mul s10, s6, s4\n";
    os << "  add s10, s10, s7\n";
    os << "  slli s10, s10, 2\n";
    os << "  add s10, s0, s10\n";
    os << "  sw s9, 0(s10)\n";
    os << "  addi s7, s7, 1\n";
    os << "  j " << colLabel << "\n";
    os << nextRowLabel << ":\n";
    os << "  addi s6, s6, 1\n";
    os << "  j " << rowLabel << "\n";
    os << endLabel << ":\n";
    os << "  ld s0, 0(sp)\n";
    os << "  ld s1, 8(sp)\n";
    os << "  ld s2, 16(sp)\n";
    os << "  ld s3, 24(sp)\n";
    os << "  ld s4, 32(sp)\n";
    os << "  ld s5, 40(sp)\n";
    os << "  ld s6, 48(sp)\n";
    os << "  ld s7, 56(sp)\n";
    os << "  ld s8, 64(sp)\n";
    os << "  ld s9, 72(sp)\n";
    os << "  ld s10, 80(sp)\n";
    os << "  ld s11, 88(sp)\n";
    os << "  addi sp, sp, 96\n";
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

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    unsigned lhsWidth = getIntegerTypeWidth(lhs->second.type);
    unsigned rhsWidth = getIntegerTypeWidth(rhs->second.type);
    if (lhsWidth != elementWidth || rhsWidth != elementWidth) {
      result.addDiagnostic("vector_add requires matching element widths");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    const char *lhsVector = "v0";
    const char *rhsVector = "v1";
    const char *sumVector = "v2";
    if (statement.getLMUL() == VectorLMUL::M2) {
      rhsVector = "v2";
      sumVector = "v4";
    } else if (statement.getLMUL() == VectorLMUL::M4) {
      rhsVector = "v4";
      sumVector = "v8";
    }

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
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth << ", "
       << getVectorLMULAssemblyName(statement.getLMUL()) << ", ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << lhsAddress << ", " << lhs->second.name << ", " << offset
       << "\n";
    os << "  add " << rhsAddress << ", " << rhs->second.name << ", " << offset
       << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v " << lhsVector << ", 0(" << lhsAddress
       << ")\n";
    os << "  vle" << elementWidth << ".v " << rhsVector << ", 0(" << rhsAddress
       << ")\n";
    os << "  vadd.vv " << sumVector << ", " << lhsVector << ", " << rhsVector
       << "\n";
    os << "  vse" << elementWidth << ".v " << sumVector << ", 0("
       << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorStridedLoad(const VectorStridedLoadStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_strided_load text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_strided_load statement");
      return;
    }

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    if (getIntegerTypeWidth(input->second.type) != elementWidth) {
      result.addDiagnostic(
          "vector_strided_load requires matching element widths");
      return;
    }

    EmittedValue stride = emitExpression(statement.getStride());
    EmittedValue length = emitExpression(statement.getLength());
    if (stride.name.empty() || length.name.empty())
      return;

    unsigned byteShift = getByteShiftForWidth(elementWidth);
    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string outputOffset = nextTempRegister();
    std::string inputOffset = nextTempRegister();
    std::string byteStride = nextTempRegister();
    std::string inputAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_strided_load");
    std::string endLabel = nextLabel(".Lvector_strided_load_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0) {
      os << "  mv " << outputOffset << ", " << index << "\n";
      os << "  mv " << byteStride << ", " << stride.name << "\n";
    } else {
      os << "  slli " << outputOffset << ", " << index << ", " << byteShift
         << "\n";
      os << "  slli " << byteStride << ", " << stride.name << ", " << byteShift
         << "\n";
    }
    os << "  mul " << inputOffset << ", " << index << ", " << stride.name
       << "\n";
    if (byteShift != 0)
      os << "  slli " << inputOffset << ", " << inputOffset << ", " << byteShift
         << "\n";
    os << "  add " << inputAddress << ", " << input->second.name << ", "
       << inputOffset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << outputOffset << "\n";
    os << "  vlse" << elementWidth << ".v v0, (" << inputAddress << "), "
       << byteStride << "\n";
    os << "  vse" << elementWidth << ".v v0, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorIndexedLoad(const VectorIndexedLoadStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_indexed_load text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    auto indices = variables.find(statement.getIndices());
    if (output == variables.end() || input == variables.end() ||
        indices == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_indexed_load statement");
      return;
    }
    if (getIntegerTypeWidth(output->second.type) != 32 ||
        getIntegerTypeWidth(input->second.type) != 32 ||
        getIntegerTypeWidth(indices->second.type) != 32) {
      result.addDiagnostic(
          "vector_indexed_load currently requires ptr<i32> data and indices");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string indexAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_indexed_load");
    std::string endLabel = nextLabel(".Lvector_indexed_load_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    os << "  add " << indexAddress << ", " << indices->second.name << ", "
       << offset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle32.v v0, 0(" << indexAddress << ")\n";
    os << "  vsll.vi v0, v0, 2\n";
    os << "  vluxei32.v v1, (" << input->second.name << "), v0\n";
    os << "  vse32.v v1, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorStridedStore(const VectorStridedStoreStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_strided_store text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto base = variables.find(statement.getBase());
    auto values = variables.find(statement.getValues());
    if (base == variables.end() || values == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_strided_store statement");
      return;
    }
    if (base->second.type != "ptr<i32>" || values->second.type != "ptr<i32>") {
      result.addDiagnostic(
          "vector_strided_store currently requires ptr<i32> data buffers");
      return;
    }

    EmittedValue stride = emitExpression(statement.getStride());
    EmittedValue length = emitExpression(statement.getLength());
    if (stride.name.empty() || length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string valuesOffset = nextTempRegister();
    std::string baseOffset = nextTempRegister();
    std::string byteStride = nextTempRegister();
    std::string valuesAddress = nextTempRegister();
    std::string baseAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_strided_store");
    std::string endLabel = nextLabel(".Lvector_strided_store_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << valuesOffset << ", " << index << ", 2\n";
    os << "  mul " << baseOffset << ", " << index << ", " << stride.name
       << "\n";
    os << "  slli " << baseOffset << ", " << baseOffset << ", 2\n";
    os << "  slli " << byteStride << ", " << stride.name << ", 2\n";
    os << "  add " << valuesAddress << ", " << values->second.name << ", "
       << valuesOffset << "\n";
    os << "  add " << baseAddress << ", " << base->second.name << ", "
       << baseOffset << "\n";
    os << "  vle32.v v0, 0(" << valuesAddress << ")\n";
    os << "  vsse32.v v0, (" << baseAddress << "), " << byteStride << "\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorIndexedStore(const VectorIndexedStoreStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_indexed_store text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto base = variables.find(statement.getBase());
    auto values = variables.find(statement.getValues());
    auto indices = variables.find(statement.getIndices());
    if (base == variables.end() || values == variables.end() ||
        indices == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_indexed_store statement");
      return;
    }
    if (base->second.type != "ptr<i32>" || values->second.type != "ptr<i32>" ||
        indices->second.type != "ptr<i32>") {
      result.addDiagnostic(
          "vector_indexed_store currently requires ptr<i32> data and indices");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string valuesAddress = nextTempRegister();
    std::string indexAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_indexed_store");
    std::string endLabel = nextLabel(".Lvector_indexed_store_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    os << "  add " << valuesAddress << ", " << values->second.name << ", "
       << offset << "\n";
    os << "  add " << indexAddress << ", " << indices->second.name << ", "
       << offset << "\n";
    os << "  vle32.v v0, 0(" << valuesAddress << ")\n";
    os << "  vle32.v v1, 0(" << indexAddress << ")\n";
    os << "  vsll.vi v1, v1, 2\n";
    os << "  vsuxei32.v v0, (" << base->second.name << "), v1\n";
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

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    if (getIntegerTypeWidth(input->second.type) != elementWidth) {
      result.addDiagnostic("vector_copy requires matching element widths");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string inputAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_copy");
    std::string endLabel = nextLabel(".Lvector_copy_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << inputAddress << ", " << input->second.name << ", "
       << offset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v v0, 0(" << inputAddress << ")\n";
    os << "  vse" << elementWidth << ".v v0, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorScale(const VectorScaleStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_scale text lowering is only available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_scale statement");
      return;
    }

    EmittedValue factor = emitExpression(statement.getFactor());
    EmittedValue length = emitExpression(statement.getLength());
    if (factor.name.empty() || length.name.empty())
      return;

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    if (getIntegerTypeWidth(input->second.type) != elementWidth) {
      result.addDiagnostic("vector_scale requires matching element widths");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string inputAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_scale");
    std::string endLabel = nextLabel(".Lvector_scale_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << inputAddress << ", " << input->second.name << ", "
       << offset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v v0, 0(" << inputAddress << ")\n";
    os << "  vmul.vx v1, v0, " << factor.name << "\n";
    os << "  vse" << elementWidth << ".v v1, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorMul(const VectorMulStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_mul text lowering is only available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_mul statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    if (getIntegerTypeWidth(lhs->second.type) != elementWidth ||
        getIntegerTypeWidth(rhs->second.type) != elementWidth) {
      result.addDiagnostic("vector_mul requires matching element widths");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string lhsAddress = nextTempRegister();
    std::string rhsAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_mul");
    std::string endLabel = nextLabel(".Lvector_mul_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << lhsAddress << ", " << lhs->second.name << ", " << offset
       << "\n";
    os << "  add " << rhsAddress << ", " << rhs->second.name << ", " << offset
       << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v v0, 0(" << lhsAddress << ")\n";
    os << "  vle" << elementWidth << ".v v1, 0(" << rhsAddress << ")\n";
    os << "  vmul.vv v2, v0, v1\n";
    os << "  vse" << elementWidth << ".v v2, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorWidenAdd(const VectorWidenAddStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_widen_add_i16_i32 text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_widen_add_i16_i32 statement");
      return;
    }
    if (getIntegerTypeWidth(output->second.type) != 32 ||
        getIntegerTypeWidth(lhs->second.type) != 16 ||
        getIntegerTypeWidth(rhs->second.type) != 16) {
      result.addDiagnostic(
          "vector_widen_add_i16_i32 requires ptr<i32>, ptr<i16>, ptr<i16>");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string inputOffset = nextTempRegister();
    std::string outputOffset = nextTempRegister();
    std::string lhsAddress = nextTempRegister();
    std::string rhsAddress = nextTempRegister();
    std::string outputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_widen_add_i16_i32");
    std::string endLabel = nextLabel(".Lvector_widen_add_i16_i32_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e16, m1, ta, ma\n";
    os << "  slli " << inputOffset << ", " << index << ", 1\n";
    os << "  slli " << outputOffset << ", " << index << ", 2\n";
    os << "  add " << lhsAddress << ", " << lhs->second.name << ", "
       << inputOffset << "\n";
    os << "  add " << rhsAddress << ", " << rhs->second.name << ", "
       << inputOffset << "\n";
    os << "  add " << outputAddress << ", " << output->second.name << ", "
       << outputOffset << "\n";
    os << "  vle16.v v0, 0(" << lhsAddress << ")\n";
    os << "  vle16.v v1, 0(" << rhsAddress << ")\n";
    os << "  vwadd.vv v2, v0, v1\n";
    os << "  vse32.v v2, 0(" << outputAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  StringRef getVectorSelectSourceName(VectorSelectPredicate predicate) {
    switch (predicate) {
    case VectorSelectPredicate::LT:
      return "vector_select_lt";
    case VectorSelectPredicate::LE:
      return "vector_select_le";
    case VectorSelectPredicate::GT:
      return "vector_select_gt";
    case VectorSelectPredicate::GE:
      return "vector_select_ge";
    case VectorSelectPredicate::EQ:
      return "vector_select_eq";
    case VectorSelectPredicate::NE:
      return "vector_select_ne";
    case VectorSelectPredicate::ULT:
      return "vector_select_ult";
    case VectorSelectPredicate::ULE:
      return "vector_select_ule";
    case VectorSelectPredicate::UGT:
      return "vector_select_ugt";
    case VectorSelectPredicate::UGE:
      return "vector_select_uge";
    }
    return "vector_select_unknown";
  }

  void emitVectorSelectCompareTo(VectorSelectPredicate predicate,
                                 StringRef destMask, StringRef lhsVector,
                                 StringRef rhsVector) {
    switch (predicate) {
    case VectorSelectPredicate::LT:
      os << "  vmslt.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::LE:
      os << "  vmsle.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::GT:
      os << "  vmslt.vv " << destMask << ", " << rhsVector << ", " << lhsVector
         << "\n";
      return;
    case VectorSelectPredicate::GE:
      os << "  vmsle.vv " << destMask << ", " << rhsVector << ", " << lhsVector
         << "\n";
      return;
    case VectorSelectPredicate::EQ:
      os << "  vmseq.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::NE:
      os << "  vmsne.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::ULT:
      os << "  vmsltu.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::ULE:
      os << "  vmsleu.vv " << destMask << ", " << lhsVector << ", " << rhsVector
         << "\n";
      return;
    case VectorSelectPredicate::UGT:
      os << "  vmsltu.vv " << destMask << ", " << rhsVector << ", " << lhsVector
         << "\n";
      return;
    case VectorSelectPredicate::UGE:
      os << "  vmsleu.vv " << destMask << ", " << rhsVector << ", " << lhsVector
         << "\n";
      return;
    }
  }

  void emitVectorSelectCompare(VectorSelectPredicate predicate) {
    emitVectorSelectCompareTo(predicate, "v0", "v1", "v2");
  }

  void emitVectorSelect(const VectorSelectStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_select text lowering is only available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    auto trueValues = variables.find(statement.getTrueValues());
    auto falseValues = variables.find(statement.getFalseValues());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end() || trueValues == variables.end() ||
        falseValues == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_select statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    unsigned elementWidth = getIntegerTypeWidth(output->second.type);
    if (getIntegerTypeWidth(lhs->second.type) != elementWidth ||
        getIntegerTypeWidth(rhs->second.type) != elementWidth ||
        getIntegerTypeWidth(trueValues->second.type) != elementWidth ||
        getIntegerTypeWidth(falseValues->second.type) != elementWidth) {
      result.addDiagnostic("vector_select requires matching element widths");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string lhsAddress = nextTempRegister();
    std::string rhsAddress = nextTempRegister();
    std::string trueAddress = nextTempRegister();
    std::string falseAddress = nextTempRegister();
    std::string labelBase =
        ".L" + getVectorSelectSourceName(statement.getPredicate()).str();
    std::string loopLabel = nextLabel(labelBase);
    std::string endLabel = nextLabel(labelBase + "_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << lhsAddress << ", " << lhs->second.name << ", " << offset
       << "\n";
    os << "  add " << rhsAddress << ", " << rhs->second.name << ", " << offset
       << "\n";
    os << "  add " << trueAddress << ", " << trueValues->second.name << ", "
       << offset << "\n";
    os << "  add " << falseAddress << ", " << falseValues->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v v1, 0(" << lhsAddress << ")\n";
    os << "  vle" << elementWidth << ".v v2, 0(" << rhsAddress << ")\n";
    os << "  vle" << elementWidth << ".v v3, 0(" << trueAddress << ")\n";
    os << "  vle" << elementWidth << ".v v4, 0(" << falseAddress << ")\n";
    emitVectorSelectCompare(statement.getPredicate());
    os << "  vmerge.vvm v5, v4, v3, v0\n";
    os << "  add " << lhsAddress << ", " << output->second.name << ", "
       << offset << "\n";
    os << "  vse" << elementWidth << ".v v5, 0(" << lhsAddress << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  StringRef getVectorMaskedBinaryInstruction(VectorMaskedBinaryOp op) const {
    switch (op) {
    case VectorMaskedBinaryOp::Add:
      return "vadd.vv";
    case VectorMaskedBinaryOp::Sub:
      return "vsub.vv";
    case VectorMaskedBinaryOp::Mul:
      return "vmul.vv";
    }
    return "vadd.vv";
  }

  void rememberVectorMask(const VectorMaskStmtAST &statement) {
    if (masks.count(statement.getMask()) ||
        logicalMasks.count(statement.getMask())) {
      result.addDiagnostic("duplicate vector mask '" + statement.getMask() +
                           "'");
      return;
    }
    masks[statement.getMask()] = &statement;
  }

  void rememberVectorMaskLogical(const VectorMaskLogicalStmtAST &statement) {
    if (masks.count(statement.getResult()) ||
        logicalMasks.count(statement.getResult())) {
      result.addDiagnostic("duplicate vector mask '" + statement.getResult() +
                           "'");
      return;
    }
    logicalMasks[statement.getResult()] = &statement;
  }

  bool emitCompareMaskInto(const VectorMaskStmtAST &statement,
                           StringRef destMask, StringRef offset,
                           StringRef address0, StringRef address1) {
    auto maskLHS = variables.find(statement.getLHS());
    auto maskRHS = variables.find(statement.getRHS());
    if (maskLHS == variables.end() || maskRHS == variables.end()) {
      result.addDiagnostic("unknown buffer in vector mask '" +
                           statement.getMask() + "'");
      return false;
    }

    os << "  add " << address0 << ", " << maskLHS->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << maskRHS->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v8, 0(" << address0 << ")\n";
    os << "  vle32.v v9, 0(" << address1 << ")\n";
    emitVectorSelectCompareTo(statement.getPredicate(), destMask, "v8", "v9");
    return true;
  }

  bool emitMaskIntoVectorRegister(StringRef maskName, StringRef destMask,
                                  StringRef offset, StringRef address0,
                                  StringRef address1) {
    auto compareMask = masks.find(maskName.str());
    if (compareMask != masks.end())
      return emitCompareMaskInto(*compareMask->second, destMask, offset,
                                 address0, address1);

    auto logicalMask = logicalMasks.find(maskName.str());
    if (logicalMask == logicalMasks.end()) {
      result.addDiagnostic("unknown vector mask '" + maskName.str() + "'");
      return false;
    }

    const VectorMaskLogicalStmtAST &statement = *logicalMask->second;
    if (statement.getOp() == VectorMaskLogicalOp::Not) {
      if (!emitMaskIntoVectorRegister(statement.getLHS(), destMask, offset,
                                      address0, address1))
        return false;
      os << "  vmnand.mm " << destMask << ", " << destMask << ", " << destMask
         << "\n";
      return true;
    }

    StringRef scratchMask =
        destMask == "v1" ? StringRef("v2") : StringRef("v1");
    if (!emitMaskIntoVectorRegister(statement.getLHS(), destMask, offset,
                                    address0, address1))
      return false;
    if (!emitMaskIntoVectorRegister(statement.getRHS(), scratchMask, offset,
                                    address0, address1))
      return false;

    switch (statement.getOp()) {
    case VectorMaskLogicalOp::And:
      os << "  vmand.mm " << destMask << ", " << destMask << ", " << scratchMask
         << "\n";
      return true;
    case VectorMaskLogicalOp::Or:
      os << "  vmor.mm " << destMask << ", " << destMask << ", " << scratchMask
         << "\n";
      return true;
    case VectorMaskLogicalOp::Xor:
      os << "  vmxor.mm " << destMask << ", " << destMask << ", " << scratchMask
         << "\n";
      return true;
    case VectorMaskLogicalOp::Not:
      return true;
    }
    return false;
  }

  void emitVectorMaskedBinary(const VectorMaskedBinaryStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked binary text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    auto passthrough = variables.find(statement.getPassthrough());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end() || passthrough == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_masked_" +
          std::string(getVectorMaskedBinaryOpName(statement.getOp())) +
          " statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string address2 = nextTempRegister();
    std::string address3 = nextTempRegister();
    std::string opName = getVectorMaskedBinaryOpName(statement.getOp());
    std::string loopLabel = nextLabel(".Lvector_masked_" + opName);
    std::string endLabel = nextLabel(".Lvector_masked_" + opName + "_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  add " << address0 << ", " << lhs->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << rhs->second.name << ", " << offset
       << "\n";
    os << "  add " << address2 << ", " << passthrough->second.name << ", "
       << offset << "\n";
    os << "  add " << address3 << ", " << output->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v3, 0(" << address0 << ")\n";
    os << "  vle32.v v4, 0(" << address1 << ")\n";
    os << "  vle32.v v5, 0(" << address2 << ")\n";
    os << "  " << getVectorMaskedBinaryInstruction(statement.getOp())
       << " v6, v3, v4, v0.t\n";
    os << "  vmerge.vvm v7, v5, v6, v0\n";
    os << "  vse32.v v7, 0(" << address3 << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorMaskedStore(const VectorMaskedStoreStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_store text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_masked_store statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string address2 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_store");
    std::string endLabel = nextLabel(".Lvector_masked_store_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  add " << address0 << ", " << input->second.name << ", " << offset
       << "\n";
    os << "  add " << address2 << ", " << output->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v3, 0(" << address0 << ")\n";
    os << "  vse32.v v3, 0(" << address2 << "), v0.t\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorMaskedLoad(const VectorMaskedLoadStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_load text lowering is only available "
                           "for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    auto passthrough = variables.find(statement.getPassthrough());
    if (output == variables.end() || input == variables.end() ||
        passthrough == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_masked_load statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string address2 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_load");
    std::string endLabel = nextLabel(".Lvector_masked_load_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  add " << address0 << ", " << input->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << passthrough->second.name << ", "
       << offset << "\n";
    os << "  add " << address2 << ", " << output->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v3, 0(" << address0 << "), v0.t\n";
    os << "  vle32.v v4, 0(" << address1 << ")\n";
    os << "  vmerge.vvm v5, v4, v3, v0\n";
    os << "  vse32.v v5, 0(" << address2 << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void
  emitVectorMaskedStridedLoad(const VectorMaskedStridedLoadStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_strided_load text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    auto passthrough = variables.find(statement.getPassthrough());
    if (output == variables.end() || input == variables.end() ||
        passthrough == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_masked_strided_load statement");
      return;
    }
    if (output->second.type != "ptr<i32>" || input->second.type != "ptr<i32>" ||
        passthrough->second.type != "ptr<i32>") {
      result.addDiagnostic("vector_masked_strided_load currently requires "
                           "ptr<i32> data buffers");
      return;
    }

    EmittedValue stride = emitExpression(statement.getStride());
    EmittedValue length = emitExpression(statement.getLength());
    if (stride.name.empty() || length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string inputOffset = nextTempRegister();
    std::string byteStride = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_strided_load");
    std::string endLabel = nextLabel(".Lvector_masked_strided_load_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  mul " << inputOffset << ", " << index << ", " << stride.name
       << "\n";
    os << "  slli " << inputOffset << ", " << inputOffset << ", 2\n";
    os << "  slli " << byteStride << ", " << stride.name << ", 2\n";
    os << "  add " << address0 << ", " << input->second.name << ", "
       << inputOffset << "\n";
    os << "  add " << address1 << ", " << passthrough->second.name << ", "
       << offset << "\n";
    os << "  add " << offset << ", " << output->second.name << ", " << offset
       << "\n";
    os << "  vlse32.v v3, (" << address0 << "), " << byteStride << ", v0.t\n";
    os << "  vle32.v v4, 0(" << address1 << ")\n";
    os << "  vmerge.vvm v5, v4, v3, v0\n";
    os << "  vse32.v v5, 0(" << offset << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void
  emitVectorMaskedIndexedLoad(const VectorMaskedIndexedLoadStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_indexed_load text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    auto indices = variables.find(statement.getIndices());
    auto passthrough = variables.find(statement.getPassthrough());
    if (output == variables.end() || input == variables.end() ||
        indices == variables.end() || passthrough == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_masked_indexed_load statement");
      return;
    }
    if (output->second.type != "ptr<i32>" || input->second.type != "ptr<i32>" ||
        indices->second.type != "ptr<i32>" ||
        passthrough->second.type != "ptr<i32>") {
      result.addDiagnostic(
          "vector_masked_indexed_load currently requires ptr<i32> data and "
          "indices");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string address2 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_indexed_load");
    std::string endLabel = nextLabel(".Lvector_masked_indexed_load_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  add " << address0 << ", " << indices->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << passthrough->second.name << ", "
       << offset << "\n";
    os << "  add " << address2 << ", " << output->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v3, 0(" << address0 << ")\n";
    os << "  vsll.vi v3, v3, 2\n";
    os << "  vluxei32.v v4, (" << input->second.name << "), v3, v0.t\n";
    os << "  vle32.v v5, 0(" << address1 << ")\n";
    os << "  vmerge.vvm v6, v5, v4, v0\n";
    os << "  vse32.v v6, 0(" << address2 << ")\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorMaskedStridedStore(
      const VectorMaskedStridedStoreStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_strided_store text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto base = variables.find(statement.getBase());
    auto values = variables.find(statement.getValues());
    if (base == variables.end() || values == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_masked_strided_store statement");
      return;
    }
    if (base->second.type != "ptr<i32>" || values->second.type != "ptr<i32>") {
      result.addDiagnostic("vector_masked_strided_store currently requires "
                           "ptr<i32> data buffers");
      return;
    }

    EmittedValue stride = emitExpression(statement.getStride());
    EmittedValue length = emitExpression(statement.getLength());
    if (stride.name.empty() || length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string baseOffset = nextTempRegister();
    std::string byteStride = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_strided_store");
    std::string endLabel = nextLabel(".Lvector_masked_strided_store_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  mul " << baseOffset << ", " << index << ", " << stride.name
       << "\n";
    os << "  slli " << baseOffset << ", " << baseOffset << ", 2\n";
    os << "  slli " << byteStride << ", " << stride.name << ", 2\n";
    os << "  add " << address0 << ", " << values->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << base->second.name << ", "
       << baseOffset << "\n";
    os << "  vle32.v v3, 0(" << address0 << ")\n";
    os << "  vsse32.v v3, (" << address1 << "), " << byteStride << ", v0.t\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorMaskedIndexedStore(
      const VectorMaskedIndexedStoreStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic("vector_masked_indexed_store text lowering is only "
                           "available for RISC-V assembly");
      return;
    }

    auto base = variables.find(statement.getBase());
    auto values = variables.find(statement.getValues());
    auto indices = variables.find(statement.getIndices());
    if (base == variables.end() || values == variables.end() ||
        indices == variables.end()) {
      result.addDiagnostic(
          "unknown buffer in vector_masked_indexed_store statement");
      return;
    }
    if (base->second.type != "ptr<i32>" || values->second.type != "ptr<i32>" ||
        indices->second.type != "ptr<i32>") {
      result.addDiagnostic(
          "vector_masked_indexed_store currently requires ptr<i32> data and "
          "indices");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string address0 = nextTempRegister();
    std::string address1 = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_masked_indexed_store");
    std::string endLabel = nextLabel(".Lvector_masked_indexed_store_end");

    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e32, m1, ta, ma\n";
    os << "  slli " << offset << ", " << index << ", 2\n";
    if (!emitMaskIntoVectorRegister(statement.getMask(), "v0", offset, address0,
                                    address1))
      return;

    os << "  add " << address0 << ", " << values->second.name << ", " << offset
       << "\n";
    os << "  add " << address1 << ", " << indices->second.name << ", " << offset
       << "\n";
    os << "  vle32.v v3, 0(" << address0 << ")\n";
    os << "  vle32.v v4, 0(" << address1 << ")\n";
    os << "  vsll.vi v4, v4, 2\n";
    os << "  vsuxei32.v v3, (" << base->second.name << "), v4, v0.t\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";
  }

  void emitVectorReduceAdd(const VectorReduceAddStmtAST &statement) {
    if (dialect != TextDialect::RiscVAssembly) {
      result.addDiagnostic(
          "vector_reduce_add text lowering is only available for RISC-V "
          "assembly");
      return;
    }

    auto resultVariable = variables.find(statement.getResult());
    auto input = variables.find(statement.getInput());
    if (resultVariable == variables.end() || input == variables.end()) {
      result.addDiagnostic(
          "unknown variable or buffer in vector_reduce_add statement");
      return;
    }

    EmittedValue length = emitExpression(statement.getLength());
    if (length.name.empty())
      return;

    unsigned elementWidth = getIntegerTypeWidth(input->second.type);
    unsigned resultWidth = getIntegerTypeWidth(resultVariable->second.type);
    if (elementWidth == 64 && resultWidth != 64) {
      result.addDiagnostic("vector_reduce_add for ptr<i64> requires an i64 "
                           "accumulator/result variable");
      return;
    }
    if (elementWidth != 64 && resultWidth != 32) {
      result.addDiagnostic("vector_reduce_add for sub-i64 inputs currently "
                           "uses an i32 accumulator/result variable");
      return;
    }
    unsigned byteShift = getByteShiftForWidth(elementWidth);

    std::string accumulator = nextTempRegister();
    std::string index = nextTempRegister();
    std::string vl = nextTempRegister();
    std::string offset = nextTempRegister();
    std::string inputAddress = nextTempRegister();
    std::string loopLabel = nextLabel(".Lvector_reduce_add");
    std::string endLabel = nextLabel(".Lvector_reduce_add_end");

    os << "  mv " << accumulator << ", " << resultVariable->second.name << "\n";
    os << "  li " << index << ", 0\n";
    os << loopLabel << ":\n";
    os << "  bgeu " << index << ", " << length.name << ", " << endLabel << "\n";
    os << "  sub " << vl << ", " << length.name << ", " << index << "\n";
    os << "  vsetvli " << vl << ", " << vl << ", e" << elementWidth
       << ", m1, ta, ma\n";
    if (byteShift == 0)
      os << "  mv " << offset << ", " << index << "\n";
    else
      os << "  slli " << offset << ", " << index << ", " << byteShift << "\n";
    os << "  add " << inputAddress << ", " << input->second.name << ", "
       << offset << "\n";
    os << "  vle" << elementWidth << ".v v0, 0(" << inputAddress << ")\n";
    os << "  vmv.s.x v1, " << accumulator << "\n";
    os << "  vredsum.vs v2, v0, v1\n";
    os << "  vmv.x.s " << accumulator << ", v2\n";
    os << "  add " << index << ", " << index << ", " << vl << "\n";
    os << "  j " << loopLabel << "\n";
    os << endLabel << ":\n";

    variables[statement.getResult()] = {accumulator,
                                        resultVariable->second.type};
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
    } else if (dialect == TextDialect::ZC)
      os << "    zc.return " << value.name << " : i32\n";
    else
      os << "    return " << value.name << " : i32\n";
  }

  void emitIf(const IfStmtAST &statement) {
    if (dialect == TextDialect::LLVMIR) {
      EmittedValue condition =
          ensureI1(emitExpression(statement.getCondition()));
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
      EmittedValue condition =
          ensureI1(emitExpression(statement.getCondition()));
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

  void emitPrintI32CallWithRegisterSave(StringRef valueRegister) {
    os << "  addi sp, sp, -128\n";
    os << "  sd ra, 0(sp)\n";
    os << "  sd a0, 8(sp)\n";
    os << "  sd a1, 16(sp)\n";
    os << "  sd a2, 24(sp)\n";
    os << "  sd a3, 32(sp)\n";
    os << "  sd a4, 40(sp)\n";
    os << "  sd a5, 48(sp)\n";
    os << "  sd a6, 56(sp)\n";
    os << "  sd a7, 64(sp)\n";
    os << "  sd t0, 72(sp)\n";
    os << "  sd t1, 80(sp)\n";
    os << "  sd t2, 88(sp)\n";
    os << "  sd t3, 96(sp)\n";
    os << "  sd t4, 104(sp)\n";
    os << "  sd t5, 112(sp)\n";
    os << "  sd t6, 120(sp)\n";
    os << "  mv a0, " << valueRegister << "\n";
    os << "  call zc_print_i32\n";
    os << "  ld ra, 0(sp)\n";
    os << "  ld a0, 8(sp)\n";
    os << "  ld a1, 16(sp)\n";
    os << "  ld a2, 24(sp)\n";
    os << "  ld a3, 32(sp)\n";
    os << "  ld a4, 40(sp)\n";
    os << "  ld a5, 48(sp)\n";
    os << "  ld a6, 56(sp)\n";
    os << "  ld a7, 64(sp)\n";
    os << "  ld t0, 72(sp)\n";
    os << "  ld t1, 80(sp)\n";
    os << "  ld t2, 88(sp)\n";
    os << "  ld t3, 96(sp)\n";
    os << "  ld t4, 104(sp)\n";
    os << "  ld t5, 112(sp)\n";
    os << "  ld t6, 120(sp)\n";
    os << "  addi sp, sp, 128\n";
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
      os << "  addw " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "-")
      os << "  subw " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "*")
      os << "  mulw " << dest << ", " << lhs << ", " << rhs << "\n";
    else if (op == "/")
      os << "  divw " << dest << ", " << lhs << ", " << rhs << "\n";
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
      os << "  subw " << dest << ", " << lhs << ", " << rhs << "\n";
      os << "  seqz " << dest << ", " << dest << "\n";
    } else if (op == "!=") {
      os << "  subw " << dest << ", " << lhs << ", " << rhs << "\n";
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
  std::map<std::string, const VectorMaskStmtAST *> masks;
  std::map<std::string, const VectorMaskLogicalStmtAST *> logicalMasks;
};

bool statementUsesPrintI32(const StmtAST &statement) {
  switch (statement.getKind()) {
  case StmtKind::PrintI32:
    return true;
  case StmtKind::If: {
    const auto &ifStatement = static_cast<const IfStmtAST &>(statement);
    for (const auto &bodyStatement : ifStatement.getThenBody())
      if (statementUsesPrintI32(*bodyStatement))
        return true;
    for (const auto &bodyStatement : ifStatement.getElseBody())
      if (statementUsesPrintI32(*bodyStatement))
        return true;
    return false;
  }
  case StmtKind::While: {
    const auto &whileStatement = static_cast<const WhileStmtAST &>(statement);
    for (const auto &bodyStatement : whileStatement.getBody())
      if (statementUsesPrintI32(*bodyStatement))
        return true;
    return false;
  }
  default:
    return false;
  }
}

bool moduleUsesPrintI32(const ModuleAST &module) {
  for (const auto &function : module.getFunctions())
    for (const auto &statement : function->getBody())
      if (statementUsesPrintI32(*statement))
        return true;
  return false;
}

void emitPrintI32Runtime(raw_ostream &os) {
  os << "  .text\n";
  os << "  .globl zc_print_i32\n";
  os << "zc_print_i32:\n";
  os << "  addi sp, sp, -64\n";
  os << "  sd ra, 56(sp)\n";
  os << "  sext.w t0, a0\n";
  os << "  addi t1, sp, 48\n";
  os << "  li t2, 10\n";
  os << "  li t3, 0\n";
  os << "  bgez t0, .Lzc_print_i32_abs_done\n";
  os << "  li t3, 1\n";
  os << "  neg t0, t0\n";
  os << ".Lzc_print_i32_abs_done:\n";
  os << "  addi t1, t1, -1\n";
  os << "  li t4, 10\n";
  os << "  sb t4, 0(t1)\n";
  os << "  li t5, 1\n";
  os << "  bnez t0, .Lzc_print_i32_digits\n";
  os << "  addi t1, t1, -1\n";
  os << "  li t4, 48\n";
  os << "  sb t4, 0(t1)\n";
  os << "  addi t5, t5, 1\n";
  os << "  j .Lzc_print_i32_sign\n";
  os << ".Lzc_print_i32_digits:\n";
  os << "  remu t4, t0, t2\n";
  os << "  divu t0, t0, t2\n";
  os << "  addi t4, t4, 48\n";
  os << "  addi t1, t1, -1\n";
  os << "  sb t4, 0(t1)\n";
  os << "  addi t5, t5, 1\n";
  os << "  bnez t0, .Lzc_print_i32_digits\n";
  os << ".Lzc_print_i32_sign:\n";
  os << "  beqz t3, .Lzc_print_i32_write\n";
  os << "  addi t1, t1, -1\n";
  os << "  li t4, 45\n";
  os << "  sb t4, 0(t1)\n";
  os << "  addi t5, t5, 1\n";
  os << ".Lzc_print_i32_write:\n";
  os << "  li a0, 1\n";
  os << "  mv a1, t1\n";
  os << "  mv a2, t5\n";
  os << "  li a7, 64\n";
  os << "  ecall\n";
  os << "  ld ra, 56(sp)\n";
  os << "  addi sp, sp, 64\n";
  os << "  ret\n";
}

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

  if (dialect == TextDialect::RiscVAssembly && moduleUsesPrintI32(module))
    emitPrintI32Runtime(os);

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
