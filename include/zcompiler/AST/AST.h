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
  Call,
  Load,
};

enum class StmtKind {
  Let,
  Assign,
  Store,
  PrintI32,
  Return,
  If,
  While,
  VectorAdd,
  VectorCopy,
  VectorScale,
  VectorMul,
  VectorReduceAdd,
  VectorSelect,
};

enum class VectorSelectPredicate {
  LT,
  LE,
  GT,
  GE,
  EQ,
  NE,
  ULT,
  ULE,
  UGT,
  UGE,
};

const char *getVectorSelectPredicateName(VectorSelectPredicate predicate);

class ParameterAST final {
public:
  ParameterAST(std::string name, std::string type)
      : name(std::move(name)), type(std::move(type)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const;
  const std::string &getName() const { return name; }
  const std::string &getType() const { return type; }

private:
  std::string name;
  std::string type;
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

class CallExprAST final : public ExprAST {
public:
  CallExprAST(std::string callee, std::vector<std::unique_ptr<ExprAST>> args)
      : callee(std::move(callee)), args(std::move(args)) {}
  ExprKind getKind() const override { return ExprKind::Call; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getCallee() const { return callee; }
  const std::vector<std::unique_ptr<ExprAST>> &getArgs() const { return args; }

private:
  std::string callee;
  std::vector<std::unique_ptr<ExprAST>> args;
};

class LoadExprAST final : public ExprAST {
public:
  LoadExprAST(std::string bufferName, std::unique_ptr<ExprAST> index)
      : bufferName(std::move(bufferName)), index(std::move(index)) {}
  ExprKind getKind() const override { return ExprKind::Load; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getBufferName() const { return bufferName; }
  const ExprAST &getIndex() const { return *index; }

private:
  std::string bufferName;
  std::unique_ptr<ExprAST> index;
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

class AssignStmtAST final : public StmtAST {
public:
  AssignStmtAST(std::string name, std::unique_ptr<ExprAST> value)
      : name(std::move(name)), value(std::move(value)) {}
  StmtKind getKind() const override { return StmtKind::Assign; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getName() const { return name; }
  const ExprAST &getValue() const { return *value; }

private:
  std::string name;
  std::unique_ptr<ExprAST> value;
};

class StoreStmtAST final : public StmtAST {
public:
  StoreStmtAST(std::string bufferName, std::unique_ptr<ExprAST> index,
               std::unique_ptr<ExprAST> value)
      : bufferName(std::move(bufferName)), index(std::move(index)),
        value(std::move(value)) {}
  StmtKind getKind() const override { return StmtKind::Store; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getBufferName() const { return bufferName; }
  const ExprAST &getIndex() const { return *index; }
  const ExprAST &getValue() const { return *value; }

private:
  std::string bufferName;
  std::unique_ptr<ExprAST> index;
  std::unique_ptr<ExprAST> value;
};

class PrintI32StmtAST final : public StmtAST {
public:
  explicit PrintI32StmtAST(std::unique_ptr<ExprAST> value)
      : value(std::move(value)) {}
  StmtKind getKind() const override { return StmtKind::PrintI32; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const ExprAST &getValue() const { return *value; }

private:
  std::unique_ptr<ExprAST> value;
};

class VectorAddStmtAST final : public StmtAST {
public:
  VectorAddStmtAST(std::string output, std::string lhs, std::string rhs,
                   std::unique_ptr<ExprAST> length)
      : output(std::move(output)), lhs(std::move(lhs)), rhs(std::move(rhs)),
        length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorAdd; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getOutput() const { return output; }
  const std::string &getLHS() const { return lhs; }
  const std::string &getRHS() const { return rhs; }
  const ExprAST &getLength() const { return *length; }

private:
  std::string output;
  std::string lhs;
  std::string rhs;
  std::unique_ptr<ExprAST> length;
};

class VectorCopyStmtAST final : public StmtAST {
public:
  VectorCopyStmtAST(std::string output, std::string input,
                    std::unique_ptr<ExprAST> length)
      : output(std::move(output)), input(std::move(input)),
        length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorCopy; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getOutput() const { return output; }
  const std::string &getInput() const { return input; }
  const ExprAST &getLength() const { return *length; }

private:
  std::string output;
  std::string input;
  std::unique_ptr<ExprAST> length;
};

class VectorScaleStmtAST final : public StmtAST {
public:
  VectorScaleStmtAST(std::string output, std::string input,
                     std::unique_ptr<ExprAST> factor,
                     std::unique_ptr<ExprAST> length)
      : output(std::move(output)), input(std::move(input)),
        factor(std::move(factor)), length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorScale; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getOutput() const { return output; }
  const std::string &getInput() const { return input; }
  const ExprAST &getFactor() const { return *factor; }
  const ExprAST &getLength() const { return *length; }

private:
  std::string output;
  std::string input;
  std::unique_ptr<ExprAST> factor;
  std::unique_ptr<ExprAST> length;
};

class VectorMulStmtAST final : public StmtAST {
public:
  VectorMulStmtAST(std::string output, std::string lhs, std::string rhs,
                   std::unique_ptr<ExprAST> length)
      : output(std::move(output)), lhs(std::move(lhs)), rhs(std::move(rhs)),
        length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorMul; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getOutput() const { return output; }
  const std::string &getLHS() const { return lhs; }
  const std::string &getRHS() const { return rhs; }
  const ExprAST &getLength() const { return *length; }

private:
  std::string output;
  std::string lhs;
  std::string rhs;
  std::unique_ptr<ExprAST> length;
};

class VectorReduceAddStmtAST final : public StmtAST {
public:
  VectorReduceAddStmtAST(std::string result, std::string input,
                         std::unique_ptr<ExprAST> length)
      : result(std::move(result)), input(std::move(input)),
        length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorReduceAdd; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  const std::string &getResult() const { return result; }
  const std::string &getInput() const { return input; }
  const ExprAST &getLength() const { return *length; }

private:
  std::string result;
  std::string input;
  std::unique_ptr<ExprAST> length;
};

class VectorSelectStmtAST final : public StmtAST {
public:
  VectorSelectStmtAST(VectorSelectPredicate predicate, std::string output,
                      std::string lhs, std::string rhs,
                      std::string trueValues, std::string falseValues,
                      std::unique_ptr<ExprAST> length)
      : predicate(predicate), output(std::move(output)), lhs(std::move(lhs)),
        rhs(std::move(rhs)), trueValues(std::move(trueValues)),
        falseValues(std::move(falseValues)), length(std::move(length)) {}
  StmtKind getKind() const override { return StmtKind::VectorSelect; }
  void dump(llvm::raw_ostream &os, unsigned indent) const override;
  VectorSelectPredicate getPredicate() const { return predicate; }
  const std::string &getOutput() const { return output; }
  const std::string &getLHS() const { return lhs; }
  const std::string &getRHS() const { return rhs; }
  const std::string &getTrueValues() const { return trueValues; }
  const std::string &getFalseValues() const { return falseValues; }
  const ExprAST &getLength() const { return *length; }

private:
  VectorSelectPredicate predicate;
  std::string output;
  std::string lhs;
  std::string rhs;
  std::string trueValues;
  std::string falseValues;
  std::unique_ptr<ExprAST> length;
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
  FunctionAST(std::string name, std::vector<ParameterAST> parameters,
              std::string returnType,
              std::vector<std::unique_ptr<StmtAST>> body)
      : name(std::move(name)), parameters(std::move(parameters)),
        returnType(std::move(returnType)), body(std::move(body)) {}
  void dump(llvm::raw_ostream &os, unsigned indent) const;
  const std::string &getName() const { return name; }
  const std::vector<ParameterAST> &getParameters() const { return parameters; }
  const std::string &getReturnType() const { return returnType; }
  const std::vector<std::unique_ptr<StmtAST>> &getBody() const { return body; }

private:
  std::string name;
  std::vector<ParameterAST> parameters;
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
