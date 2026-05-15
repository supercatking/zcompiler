#include "zcompiler/AST/AST.h"

using namespace llvm;

namespace zc {

const char *getVectorSelectPredicateName(VectorSelectPredicate predicate) {
  switch (predicate) {
  case VectorSelectPredicate::LT:
    return "lt";
  case VectorSelectPredicate::LE:
    return "le";
  case VectorSelectPredicate::GT:
    return "gt";
  case VectorSelectPredicate::GE:
    return "ge";
  case VectorSelectPredicate::EQ:
    return "eq";
  case VectorSelectPredicate::NE:
    return "ne";
  case VectorSelectPredicate::ULT:
    return "ult";
  case VectorSelectPredicate::ULE:
    return "ule";
  case VectorSelectPredicate::UGT:
    return "ugt";
  case VectorSelectPredicate::UGE:
    return "uge";
  }
  return "unknown";
}

const char *getVectorMaskedBinaryOpName(VectorMaskedBinaryOp op) {
  switch (op) {
  case VectorMaskedBinaryOp::Add:
    return "add";
  case VectorMaskedBinaryOp::Sub:
    return "sub";
  case VectorMaskedBinaryOp::Mul:
    return "mul";
  }
  return "unknown";
}

const char *getVectorMaskLogicalOpName(VectorMaskLogicalOp op) {
  switch (op) {
  case VectorMaskLogicalOp::And:
    return "and";
  case VectorMaskLogicalOp::Or:
    return "or";
  case VectorMaskLogicalOp::Xor:
    return "xor";
  case VectorMaskLogicalOp::Not:
    return "not";
  }
  return "unknown";
}

const char *getVectorLMULName(VectorLMUL lmul) {
  switch (lmul) {
  case VectorLMUL::M1:
    return "m1";
  case VectorLMUL::M2:
    return "m2";
  case VectorLMUL::M4:
    return "m4";
  }
  return "unknown";
}

const char *getMatrixRHSLayoutName(MatrixRHSLayout layout) {
  switch (layout) {
  case MatrixRHSLayout::RowMajor:
    return "row_major";
  case MatrixRHSLayout::PackedColumns:
    return "packed_columns";
  }
  return "unknown";
}

namespace {

void writeIndent(raw_ostream &os, unsigned indent) {
  for (unsigned index = 0; index < indent; ++index)
    os << "  ";
}

} // namespace

void ParameterAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "Param name=" << name << " type=" << type << '\n';
}

void IntegerExprAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "IntegerExpr value=" << value << '\n';
}

void VariableExprAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VariableExpr name=" << name << '\n';
}

void BinaryExprAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "BinaryExpr op=\"" << op << "\"\n";
  lhs->dump(os, indent + 1);
  rhs->dump(os, indent + 1);
}

void CallExprAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "CallExpr callee=" << callee << '\n';
  for (const auto &arg : args)
    arg->dump(os, indent + 1);
}

void LoadExprAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "LoadExpr buffer=" << bufferName << '\n';
  writeIndent(os, indent + 1);
  os << "Index\n";
  index->dump(os, indent + 2);
}

void LetStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "LetStmt name=" << name << '\n';
  value->dump(os, indent + 1);
}

void AssignStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "AssignStmt name=" << name << '\n';
  value->dump(os, indent + 1);
}

void StoreStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "StoreStmt buffer=" << bufferName << '\n';
  writeIndent(os, indent + 1);
  os << "Index\n";
  index->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Value\n";
  value->dump(os, indent + 2);
}

void PrintI32StmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "PrintI32Stmt\n";
  value->dump(os, indent + 1);
}

void MatrixPackBStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "MatrixPackBStmt output=" << output << " input=" << input << '\n';
  writeIndent(os, indent + 1);
  os << "Cols\n";
  cols->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Inner\n";
  inner->dump(os, indent + 2);
}

void MatrixMultiplyStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "MatrixMultiplyStmt output=" << output << " lhs=" << lhs
     << " rhs=" << rhs << " rhs_layout=" << getMatrixRHSLayoutName(rhsLayout)
     << '\n';
  writeIndent(os, indent + 1);
  os << "Rows\n";
  rows->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Cols\n";
  cols->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Inner\n";
  inner->dump(os, indent + 2);
}

void VectorAddStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorAddStmt output=" << output << " lhs=" << lhs << " rhs=" << rhs
     << " lmul=" << getVectorLMULName(lmul) << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorStridedLoadStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorStridedLoadStmt output=" << output << " input=" << input << '\n';
  writeIndent(os, indent + 1);
  os << "Stride\n";
  stride->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorIndexedLoadStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorIndexedLoadStmt output=" << output << " input=" << input
     << " indices=" << indices << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorStridedStoreStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorStridedStoreStmt base=" << base << " values=" << values << '\n';
  writeIndent(os, indent + 1);
  os << "Stride\n";
  stride->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorIndexedStoreStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorIndexedStoreStmt base=" << base << " values=" << values
     << " indices=" << indices << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorCopyStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorCopyStmt output=" << output << " input=" << input << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorScaleStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorScaleStmt output=" << output << " input=" << input << '\n';
  writeIndent(os, indent + 1);
  os << "Factor\n";
  factor->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMulStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMulStmt output=" << output << " lhs=" << lhs << " rhs=" << rhs
     << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorWidenAddStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorWidenAddStmt output=" << output << " lhs=" << lhs
     << " rhs=" << rhs << " source=i16 dest=i32" << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorReduceAddStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorReduceAddStmt result=" << result << " input=" << input << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorSelectStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorSelectStmt predicate=" << getVectorSelectPredicateName(predicate)
     << " output=" << output << " lhs=" << lhs << " rhs=" << rhs
     << " true=" << trueValues << " false=" << falseValues << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMaskStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMaskStmt predicate=" << getVectorSelectPredicateName(predicate)
     << " mask=" << mask << " lhs=" << lhs << " rhs=" << rhs << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMaskLogicalStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMaskLogicalStmt op=" << getVectorMaskLogicalOpName(op)
     << " result=" << result << " lhs=" << lhs;
  if (op != VectorMaskLogicalOp::Not)
    os << " rhs=" << rhs;
  os << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMaskedBinaryStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMaskedBinaryStmt op=" << getVectorMaskedBinaryOpName(op)
     << " output=" << output << " lhs=" << lhs << " rhs=" << rhs
     << " mask=" << mask << " passthrough=" << passthrough << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMaskedStoreStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMaskedStoreStmt output=" << output << " input=" << input
     << " mask=" << mask << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void VectorMaskedLoadStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorMaskedLoadStmt output=" << output << " input=" << input
     << " mask=" << mask << " passthrough=" << passthrough << '\n';
  writeIndent(os, indent + 1);
  os << "Length\n";
  length->dump(os, indent + 2);
}

void ReturnStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "ReturnStmt\n";
  value->dump(os, indent + 1);
}

void IfStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "IfStmt\n";
  writeIndent(os, indent + 1);
  os << "Condition\n";
  condition->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Then\n";
  for (const auto &statement : thenBody)
    statement->dump(os, indent + 2);
  if (!elseBody.empty()) {
    writeIndent(os, indent + 1);
    os << "Else\n";
    for (const auto &statement : elseBody)
      statement->dump(os, indent + 2);
  }
}

void WhileStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "WhileStmt\n";
  writeIndent(os, indent + 1);
  os << "Condition\n";
  condition->dump(os, indent + 2);
  writeIndent(os, indent + 1);
  os << "Body\n";
  for (const auto &statement : body)
    statement->dump(os, indent + 2);
}

void FunctionAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "Function name=" << name << " return=" << returnType << '\n';
  if (!parameters.empty()) {
    writeIndent(os, indent + 1);
    os << "Params\n";
    for (const auto &parameter : parameters)
      parameter.dump(os, indent + 2);
  }
  for (const auto &statement : body)
    statement->dump(os, indent + 1);
}

void ModuleAST::dump(raw_ostream &os) const {
  os << "Module\n";
  for (const auto &function : functions)
    function->dump(os, 1);
}

} // namespace zc
