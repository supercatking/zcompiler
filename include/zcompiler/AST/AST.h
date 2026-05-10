#ifndef ZCOMPILER_AST_AST_H
#define ZCOMPILER_AST_AST_H

#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace zc {

enum class ExprKind {
  Integer,
  Variable,
  Binary,
};

enum class StmtKind {
  Let,
  Return,
  If,
  While,
};

class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual ExprKind getKind() const = 0;
  virtual void dump(llvm::raw_ostream &os, unsigned indent) const = 0;
};

class IntegerExprAST final : public ExprAST {
public:
  explicit IntegerExprAST(std::string value) : value(std::move(value)) {}
  ExprKind getKind() const override { return ExprKind::Integer; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getValue() const { return value; }

private:
  std::string value;
};

class VariableExprAST final : public ExprAST {
public:
  explicit VariableExprAST(std::string name) : name(std::move(name)) {}
  ExprKind getKind() const override { return ExprKind::Variable; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getName() const { return name; }

private:
  std::string name;
};

class BinaryExprAST final : public ExprAST {
public:
  BinaryExprAST(std::string op, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs)
      : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  ExprKind getKind() const override { return ExprKind::Binary; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getOp() const { return op; }
  const ExprAST &getLHS() const { return *lhs; }
  const ExprAST &getRHS() const { return *rhs; }

private:
  std::string op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;
};

class StmtAST {
public:
  virtual ~StmtAST() = default;
  virtual StmtKind getKind() const = 0;
  virtual void dump(llvm::raw_ostream &os, unsigned indent) const = 0;
};

class LetStmtAST final : public StmtAST {
public:
  LetStmtAST(std::string name, std::unique_ptr<ExprAST> value)
      : name(std::move(name)), value(std::move(value)) {}
  StmtKind getKind() const override { return StmtKind::Let; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getName() const { return name; }
  const ExprAST &getValue() const { return *value; }

private:
  std::string name;
  std::unique_ptr<ExprAST> value;
};

class ReturnStmtAST final : public StmtAST {
public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> value)
      : value(std::move(value)) {}
  StmtKind getKind() const override { return StmtKind::Return; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const ExprAST &getValue() const { return *value; }

private:
  std::unique_ptr<ExprAST> value;
};

class IfStmtAST final : public StmtAST {
public:
  IfStmtAST(std::unique_ptr<ExprAST> condition,
            std::vector<std::unique_ptr<StmtAST>> thenBody,
            std::vector<std::unique_ptr<StmtAST>> elseBody)
      : condition(std::move(condition)), thenBody(std::move(thenBody)),
        elseBody(std::move(elseBody)) {}
  StmtKind getKind() const override { return StmtKind::If; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const ExprAST &getCondition() const { return *condition; }
  const std::vector<std::unique_ptr<StmtAST>> &getThenBody() const {
    return thenBody;
  }
  const std::vector<std::unique_ptr<StmtAST>> &getElseBody() const {
    return elseBody;
  }

private:
  std::unique_ptr<ExprAST> condition;
  std::vector<std::unique_ptr<StmtAST>> thenBody;
  std::vector<std::unique_ptr<StmtAST>> elseBody;
};

class WhileStmtAST final : public StmtAST {
public:
  WhileStmtAST(std::unique_ptr<ExprAST> condition,
               std::vector<std::unique_ptr<StmtAST>> body)
      : condition(std::move(condition)), body(std::move(body)) {}
  StmtKind getKind() const override { return StmtKind::While; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const ExprAST &getCondition() const { return *condition; }
  const std::vector<std::unique_ptr<StmtAST>> &getBody() const { return body; }

private:
  std::unique_ptr<ExprAST> condition;
  std::vector<std::unique_ptr<StmtAST>> body;
};

class FunctionAST final {
public:
  FunctionAST(std::string name, std::string returnType,
              std::vector<std::unique_ptr<StmtAST>> body)
      : name(std::move(name)), returnType(std::move(returnType)),
        body(std::move(body)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const;
  const std::string &getName() const { return name; }
  const std::string &getReturnType() const { return returnType; }
  const std::vector<std::unique_ptr<StmtAST>> &getBody() const { return body; }

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
  const std::vector<std::unique_ptr<FunctionAST>> &getFunctions() const {
    return functions;
  }

private:
  std::vector<std::unique_ptr<FunctionAST>> functions;
};

} // namespace zc

#endif // ZCOMPILER_AST_AST_H
