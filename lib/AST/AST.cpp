#include "zcompiler/AST/AST.h"

using namespace llvm;

namespace zc {

namespace {

void writeIndent(raw_ostream &os, unsigned indent) {
  for (unsigned index = 0; index < indent; ++index)
    os << "  ";
}

} // namespace

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

void LetStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "LetStmt name=" << name << '\n';
  value->dump(os, indent + 1);
}

void ReturnStmtAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "ReturnStmt\n";
  value->dump(os, indent + 1);
}

void FunctionAST::dump(raw_ostream &os, unsigned indent) const {
  writeIndent(os, indent);
  os << "Function name=" << name << " return=" << returnType << '\n';
  for (const auto &statement : body)
    statement->dump(os, indent + 1);
}

void ModuleAST::dump(raw_ostream &os) const {
  os << "Module\n";
  for (const auto &function : functions)
    function->dump(os, 1);
}

} // namespace zc

