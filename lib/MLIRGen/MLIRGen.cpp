#include "zcompiler/MLIRGen/MLIRGen.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"

#include "llvm/ADT/STLExtras.h"

#include <map>
#include <string>

using namespace mlir;

namespace zc {
namespace {

class MLIRGenImpl {
public:
  MLIRGenImpl(MLIRContext &context, MLIRGenResult &result)
      : context(context), builder(&context), result(result) {}

  OwningOpRef<ModuleOp> generate(const ModuleAST &moduleAST) {
    context.loadDialect<arith::ArithDialect, func::FuncDialect>();

    auto module = ModuleOp::create(builder.getUnknownLoc());
    builder.setInsertionPointToStart(module.getBody());

    for (const auto &function : moduleAST.getFunctions())
      emitFunction(*function);

    return OwningOpRef<ModuleOp>(module);
  }

private:
  void emitFunction(const FunctionAST &function) {
    variables.clear();
    Location loc = builder.getUnknownLoc();
    Type i32 = builder.getI32Type();
    SmallVector<Type, 4> inputTypes(function.getParameters().size(), i32);
    auto functionType = builder.getFunctionType(inputTypes, {i32});
    auto func = func::FuncOp::create(loc, function.getName(), functionType);
    builder.insert(func);

    Block *entry = func.addEntryBlock();
    builder.setInsertionPointToStart(entry);
    for (auto [parameter, argument] :
         llvm::zip(function.getParameters(), entry->getArguments()))
      variables[parameter.getName()] = argument;

    bool hasTerminator = false;
    for (const auto &statement : function.getBody()) {
      emitStatement(*statement);
      hasTerminator = !entry->empty() && entry->back().hasTrait<OpTrait::IsTerminator>();
      if (hasTerminator)
        break;
    }

    if (!hasTerminator)
      builder.create<func::ReturnOp>(
          loc, ValueRange(builder.create<arith::ConstantIntOp>(loc, 0, 32)));

    builder.setInsertionPointAfter(func);
  }

  Value emitExpression(const ExprAST &expression) {
    switch (expression.getKind()) {
    case ExprKind::Integer:
      return emitInteger(static_cast<const IntegerExprAST &>(expression));
    case ExprKind::Variable:
      return emitVariable(static_cast<const VariableExprAST &>(expression));
    case ExprKind::Binary:
      return emitBinary(static_cast<const BinaryExprAST &>(expression));
    case ExprKind::Call:
      return emitCall(static_cast<const CallExprAST &>(expression));
    }

    result.addDiagnostic("unknown expression kind");
    return nullptr;
  }

  Value emitInteger(const IntegerExprAST &expression) {
    return builder.create<arith::ConstantIntOp>(
        builder.getUnknownLoc(), std::stoll(expression.getValue()), 32);
  }

  Value emitVariable(const VariableExprAST &expression) {
    auto found = variables.find(expression.getName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown variable '" + expression.getName() + "'");
      return nullptr;
    }
    return found->second;
  }

  Value emitBinary(const BinaryExprAST &expression) {
    Value lhs = emitExpression(expression.getLHS());
    Value rhs = emitExpression(expression.getRHS());
    if (!lhs || !rhs)
      return nullptr;

    Location loc = builder.getUnknownLoc();
    StringRef op = expression.getOp();
    if (op == "+")
      return builder.create<arith::AddIOp>(loc, lhs, rhs);
    if (op == "-")
      return builder.create<arith::SubIOp>(loc, lhs, rhs);
    if (op == "*")
      return builder.create<arith::MulIOp>(loc, lhs, rhs);
    if (op == "/")
      return builder.create<arith::DivSIOp>(loc, lhs, rhs);
    if (op == "<")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::slt, lhs,
                                           rhs);
    if (op == "<=")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::sle, lhs,
                                           rhs);
    if (op == ">")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::sgt, lhs,
                                           rhs);
    if (op == ">=")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::sge, lhs,
                                           rhs);
    if (op == "==")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::eq, lhs,
                                           rhs);
    if (op == "!=")
      return builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::ne, lhs,
                                           rhs);

    result.addDiagnostic("unsupported binary operator '" + expression.getOp() +
                         "'");
    return nullptr;
  }

  Value emitCall(const CallExprAST &expression) {
    SmallVector<Value, 4> args;
    for (const auto &arg : expression.getArgs()) {
      Value value = emitExpression(*arg);
      if (!value)
        return nullptr;
      args.push_back(value);
    }

    auto call = builder.create<func::CallOp>(
        builder.getUnknownLoc(), expression.getCallee(),
        TypeRange(builder.getI32Type()), ValueRange(args));
    return call.getResult(0);
  }

  void emitStatement(const StmtAST &statement) {
    switch (statement.getKind()) {
    case StmtKind::Let:
      emitLet(static_cast<const LetStmtAST &>(statement));
      return;
    case StmtKind::Assign:
      emitAssign(static_cast<const AssignStmtAST &>(statement));
      return;
    case StmtKind::Return:
      emitReturn(static_cast<const ReturnStmtAST &>(statement));
      return;
    case StmtKind::If:
      result.addDiagnostic("MLIR API generation for if is planned for Phase 19");
      return;
    case StmtKind::While:
      result.addDiagnostic(
          "MLIR API generation for while is planned for Phase 19");
      return;
    }
  }

  void emitLet(const LetStmtAST &statement) {
    Value value = emitExpression(statement.getValue());
    if (value)
      variables[statement.getName()] = value;
  }

  void emitAssign(const AssignStmtAST &statement) {
    Value value = emitExpression(statement.getValue());
    if (value)
      variables[statement.getName()] = value;
  }

  void emitReturn(const ReturnStmtAST &statement) {
    Value value = emitExpression(statement.getValue());
    if (value)
      builder.create<func::ReturnOp>(builder.getUnknownLoc(),
                                     ValueRange(value));
  }

  MLIRContext &context;
  OpBuilder builder;
  MLIRGenResult &result;
  std::map<std::string, Value> variables;
};

} // namespace

OwningOpRef<ModuleOp> generateMLIRModule(MLIRContext &context,
                                         const ModuleAST &module,
                                         MLIRGenResult &result) {
  MLIRGenImpl generator(context, result);
  return generator.generate(module);
}

} // namespace zc
