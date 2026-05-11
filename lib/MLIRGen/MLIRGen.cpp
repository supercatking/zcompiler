#include "zcompiler/MLIRGen/MLIRGen.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/IR/AffineMap.h"
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
    context.loadDialect<arith::ArithDialect, func::FuncDialect,
                        memref::MemRefDialect, scf::SCFDialect,
                        vector::VectorDialect>();

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
    SmallVector<Type, 4> inputTypes;
    for (const ParameterAST &parameter : function.getParameters())
      inputTypes.push_back(getType(parameter.getType()));
    auto functionType =
        builder.getFunctionType(inputTypes, {getType(function.getReturnType())});
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
    case ExprKind::Load:
      return emitLoad(static_cast<const LoadExprAST &>(expression));
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

  Value emitLoad(const LoadExprAST &expression) {
    auto found = variables.find(expression.getBufferName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown buffer '" + expression.getBufferName() +
                           "'");
      return nullptr;
    }

    Value index = ensureIndex(emitExpression(expression.getIndex()));
    if (!index)
      return nullptr;

    return builder.create<memref::LoadOp>(builder.getUnknownLoc(),
                                          found->second, ValueRange(index));
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
      result.addDiagnostic(
          "MLIR API generation for print_i32 is planned after the RISC-V "
          "runtime slice");
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
    case StmtKind::VectorAdd:
      emitVectorAdd(static_cast<const VectorAddStmtAST &>(statement));
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
    case StmtKind::VectorReduceAdd:
      emitVectorReduceAdd(
          static_cast<const VectorReduceAddStmtAST &>(statement));
      return;
    case StmtKind::VectorSelect:
      emitVectorSelect(static_cast<const VectorSelectStmtAST &>(statement));
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

  void emitStore(const StoreStmtAST &statement) {
    auto found = variables.find(statement.getBufferName());
    if (found == variables.end()) {
      result.addDiagnostic("unknown buffer '" + statement.getBufferName() +
                           "'");
      return;
    }

    Value index = ensureIndex(emitExpression(statement.getIndex()));
    Value value = emitExpression(statement.getValue());
    if (!index || !value)
      return;

    builder.create<memref::StoreOp>(builder.getUnknownLoc(), value,
                                    found->second, ValueRange(index));
  }

  scf::ForOp createMaskedVectorLoop(Value upperBound, Value &step) {
    Location loc = builder.getUnknownLoc();
    Value lowerBound = builder.create<arith::ConstantIndexOp>(loc, 0);
    step = builder.create<arith::ConstantIndexOp>(loc, 4);
    return builder.create<scf::ForOp>(loc, lowerBound, upperBound, step);
  }

  struct MaskedVectorAccess {
    Location loc;
    VectorType vectorType;
    Value zero;
    SmallVector<Value, 1> indices;
    Value mask;
    AffineMapAttr permutationMap;
    ArrayAttr inBounds;
  };

  MaskedVectorAccess createMaskedVectorAccess(Value upperBound, Value step,
                                              Value index) {
    Location loc = builder.getUnknownLoc();
    MaskedVectorAccess access{
        loc,
        VectorType::get({4}, builder.getI32Type()),
        builder.create<arith::ConstantIntOp>(loc, 0, 32),
        SmallVector<Value, 1>{index},
        nullptr,
        AffineMapAttr::get(AffineMap::getMinorIdentityMap(1, 1, &context)),
        builder.getBoolArrayAttr({false}),
    };

    auto maskType = VectorType::get({4}, builder.getI1Type());
    Value remaining = builder.create<arith::SubIOp>(loc, upperBound, index);
    Value activeLanes = builder.create<arith::MinUIOp>(loc, remaining, step);
    access.mask = builder.create<vector::CreateMaskOp>(
        loc, maskType, ValueRange(activeLanes));
    return access;
  }

  Value emitVectorRead(Value buffer, const MaskedVectorAccess &access) {
    return builder.create<vector::TransferReadOp>(
        access.loc, access.vectorType, buffer, ValueRange(access.indices),
        access.permutationMap, access.zero, access.mask, access.inBounds);
  }

  void emitVectorWrite(Value vector, Value buffer,
                       const MaskedVectorAccess &access) {
    builder.create<vector::TransferWriteOp>(
        access.loc, vector, buffer, ValueRange(access.indices),
        access.permutationMap, access.mask, access.inBounds);
  }

  void emitVectorAdd(const VectorAddStmtAST &statement) {
    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_add statement");
      return;
    }

    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!upperBound)
      return;

    Value step;
    auto forOp = createMaskedVectorLoop(upperBound, step);

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value lhsVector = emitVectorRead(lhs->second, access);
    Value rhsVector = emitVectorRead(rhs->second, access);
    Value sum = builder.create<arith::AddIOp>(access.loc, lhsVector, rhsVector);
    emitVectorWrite(sum, output->second, access);
  }

  void emitVectorCopy(const VectorCopyStmtAST &statement) {
    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_copy statement");
      return;
    }

    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!upperBound)
      return;

    Value step;
    auto forOp = createMaskedVectorLoop(upperBound, step);

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value inputVector = emitVectorRead(input->second, access);
    emitVectorWrite(inputVector, output->second, access);
  }

  void emitVectorScale(const VectorScaleStmtAST &statement) {
    auto output = variables.find(statement.getOutput());
    auto input = variables.find(statement.getInput());
    if (output == variables.end() || input == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_scale statement");
      return;
    }

    Value factor = emitExpression(statement.getFactor());
    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!factor || !upperBound)
      return;

    Value step;
    auto forOp = createMaskedVectorLoop(upperBound, step);

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value inputVector = emitVectorRead(input->second, access);
    Value factorVector = builder.create<vector::BroadcastOp>(
        access.loc, access.vectorType, factor);
    Value scaled =
        builder.create<arith::MulIOp>(access.loc, inputVector, factorVector);
    emitVectorWrite(scaled, output->second, access);
  }

  void emitVectorMul(const VectorMulStmtAST &statement) {
    auto output = variables.find(statement.getOutput());
    auto lhs = variables.find(statement.getLHS());
    auto rhs = variables.find(statement.getRHS());
    if (output == variables.end() || lhs == variables.end() ||
        rhs == variables.end()) {
      result.addDiagnostic("unknown buffer in vector_mul statement");
      return;
    }

    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!upperBound)
      return;

    Value step;
    auto forOp = createMaskedVectorLoop(upperBound, step);

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value lhsVector = emitVectorRead(lhs->second, access);
    Value rhsVector = emitVectorRead(rhs->second, access);
    Value product =
        builder.create<arith::MulIOp>(access.loc, lhsVector, rhsVector);
    emitVectorWrite(product, output->second, access);
  }

  arith::CmpIPredicate getMLIRPredicate(VectorSelectPredicate predicate) {
    switch (predicate) {
    case VectorSelectPredicate::LT:
      return arith::CmpIPredicate::slt;
    case VectorSelectPredicate::LE:
      return arith::CmpIPredicate::sle;
    case VectorSelectPredicate::GT:
      return arith::CmpIPredicate::sgt;
    case VectorSelectPredicate::GE:
      return arith::CmpIPredicate::sge;
    case VectorSelectPredicate::EQ:
      return arith::CmpIPredicate::eq;
    case VectorSelectPredicate::NE:
      return arith::CmpIPredicate::ne;
    }
    return arith::CmpIPredicate::eq;
  }

  void emitVectorSelect(const VectorSelectStmtAST &statement) {
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

    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!upperBound)
      return;

    Value step;
    auto forOp = createMaskedVectorLoop(upperBound, step);

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value lhsVector = emitVectorRead(lhs->second, access);
    Value rhsVector = emitVectorRead(rhs->second, access);
    Value trueVector = emitVectorRead(trueValues->second, access);
    Value falseVector = emitVectorRead(falseValues->second, access);
    Value mask = builder.create<arith::CmpIOp>(
        access.loc, getMLIRPredicate(statement.getPredicate()), lhsVector,
        rhsVector);
    Value selected =
        builder.create<arith::SelectOp>(access.loc, mask, trueVector, falseVector);
    emitVectorWrite(selected, output->second, access);
  }

  void emitVectorReduceAdd(const VectorReduceAddStmtAST &statement) {
    auto resultVariable = variables.find(statement.getResult());
    auto input = variables.find(statement.getInput());
    if (resultVariable == variables.end() || input == variables.end()) {
      result.addDiagnostic(
          "unknown variable or buffer in vector_reduce_add statement");
      return;
    }

    Value upperBound = ensureIndex(emitExpression(statement.getLength()));
    if (!upperBound)
      return;

    Location loc = builder.getUnknownLoc();
    Value lowerBound = builder.create<arith::ConstantIndexOp>(loc, 0);
    Value step = builder.create<arith::ConstantIndexOp>(loc, 4);
    auto forOp = builder.create<scf::ForOp>(
        loc, lowerBound, upperBound, step, ValueRange(resultVariable->second));

    OpBuilder::InsertionGuard guard(builder);
    builder.setInsertionPointToStart(forOp.getBody());
    MaskedVectorAccess access =
        createMaskedVectorAccess(upperBound, step, forOp.getInductionVar());

    Value inputVector = emitVectorRead(input->second, access);
    Value loopAccumulator = forOp.getRegionIterArg(0);
    Value reduced = builder.create<vector::ReductionOp>(
        access.loc, builder.getI32Type(), vector::CombiningKind::ADD,
        inputVector, loopAccumulator, arith::FastMathFlags::none);
    builder.create<scf::YieldOp>(access.loc, ValueRange(reduced));

    variables[statement.getResult()] = forOp.getResult(0);
  }

  void emitReturn(const ReturnStmtAST &statement) {
    Value value = emitExpression(statement.getValue());
    if (value)
      builder.create<func::ReturnOp>(builder.getUnknownLoc(),
                                     ValueRange(value));
  }

  Type getType(StringRef sourceType) {
    if (sourceType == "ptr<i32>")
      return MemRefType::get({ShapedType::kDynamic}, builder.getI32Type());
    return builder.getI32Type();
  }

  Value ensureIndex(Value value) {
    if (!value)
      return nullptr;
    if (value.getType().isIndex())
      return value;
    if (value.getType().isInteger(32))
      return builder.create<arith::IndexCastOp>(
          builder.getUnknownLoc(), builder.getIndexType(), value);
    result.addDiagnostic("expected i32 or index expression for memory index");
    return nullptr;
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
