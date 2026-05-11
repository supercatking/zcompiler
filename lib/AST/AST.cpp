#include "zcompiler/AST/AST.h"

using namespace llvm;

namespace zc {

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

void VectorAddStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorAddStmt output=" << output << " lhs=" << lhs << " rhs=" << rhs
     << '\n';
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

void VectorReduceAddStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "VectorReduceAddStmt result=" << result << " input=" << input << '\n';
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
