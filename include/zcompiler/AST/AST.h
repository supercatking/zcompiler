#ifndef ZCOMPILER_AST_AST_H
#define ZCOMPILER_AST_AST_H

#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace zc {

class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual void dump(llvm::raw_ostream &os, unsigned indent) const = 0;
};

class IntegerExprAST final : public ExprAST {
public:
  explicit IntegerExprAST(std::string value) : value(std::move(value)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const override;

private:
  std::string value;
};

class VariableExprAST final : public ExprAST {
public:
  explicit VariableExprAST(std::string name) : name(std::move(name)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const override;

private:
  std::string name;
};

class BinaryExprAST final : public ExprAST {
public:
  BinaryExprAST(std::string op, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs)
      : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const override;

private:
  std::string op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;
};

class StmtAST {
public:
  virtual ~StmtAST() = default;
  virtual void dump(llvm::raw_ostream &os, unsigned indent) const = 0;
};

class LetStmtAST final : public StmtAST {
public:
  LetStmtAST(std::string name, std::unique_ptr<ExprAST> value)
      : name(std::move(name)), value(std::move(value)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const override;

private:
  std::string name;
  std::unique_ptr<ExprAST> value;
};

class ReturnStmtAST final : public StmtAST {
public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> value)
      : value(std::move(value)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const override;

private:
  std::unique_ptr<ExprAST> value;
};

class FunctionAST final {
public:
  FunctionAST(std::string name, std::string returnType,
              std::vector<std::unique_ptr<StmtAST>> body)
      : name(std::move(name)), returnType(std::move(returnType)),
        body(std::move(body)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const;

private:
  std::string name;
  std::string returnType;
  std::vector<std::unique_ptr<StmtAST>> body;
};

class ModuleAST final {
public:
  explicit ModuleAST(std::vector<std::unique_ptr<FunctionAST>> functions)
      : functions(std::move(functions)) {}
  void dump(llvm::raw_ostream &os) const;

private:
  std::vector<std::unique_ptr<FunctionAST>> functions;
};

} // namespace zc

#endif // ZCOMPILER_AST_AST_H
