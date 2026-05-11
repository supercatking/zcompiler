#include "zcompiler/Parser/Parser.h"

#include "llvm/Support/FormatVariadic.h"

using namespace llvm;

namespace zc {

Parser::Parser(ArrayRef<Token> tokens) : tokens(tokens) {}

std::unique_ptr<ModuleAST> Parser::parseModule() {
  std::vector<std::unique_ptr<FunctionAST>> functions;

  while (!check(TokenKind::EndOfFile)) {
    auto function = parseFunction();
    if (!function)
      return nullptr;
    functions.push_back(std::move(function));
  }

  return std::make_unique<ModuleAST>(std::move(functions));
}

const Token &Parser::peek(unsigned offset) const {
  unsigned index = current + offset;
  if (index >= tokens.size())
    return tokens.back();
  return tokens[index];
}

const Token &Parser::advance() {
  const Token &token = peek();
  if (!check(TokenKind::EndOfFile))
    ++current;
  return token;
}

bool Parser::check(TokenKind kind) const { return peek().kind == kind; }

bool Parser::match(TokenKind kind) {
  if (!check(kind))
    return false;
  advance();
  return true;
}

bool Parser::expect(TokenKind kind, StringRef message) {
  if (match(kind))
    return true;
  reportAtCurrent(message);
  return false;
}

void Parser::reportAtCurrent(StringRef message) {
  const Token &token = peek();
  diagnostics.push_back(
      formatv("{0}:{1}: {2}", token.line, token.column, message).str());
}

std::unique_ptr<FunctionAST> Parser::parseFunction() {
  if (!expect(TokenKind::KwFunc, "expected 'func' to start function"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected function name");
    return nullptr;
  }
  std::string name = advance().lexeme;

  if (!expect(TokenKind::LParen, "expected '(' after function name"))
    return nullptr;

  std::vector<ParameterAST> parameters;
  if (!parseParameters(parameters) ||
      !expect(TokenKind::Arrow, "expected '->' before return type"))
    return nullptr;

  std::string returnType;
  if (!parseType(returnType)) {
    reportAtCurrent("expected return type 'i32'");
    return nullptr;
  }

  if (!expect(TokenKind::LBrace, "expected '{' before function body"))
    return nullptr;

  std::vector<std::unique_ptr<StmtAST>> body;
  if (!parseBlock(body))
    return nullptr;

  return std::make_unique<FunctionAST>(std::move(name), std::move(parameters),
                                       std::move(returnType), std::move(body));
}

bool Parser::parseType(std::string &type) {
  if (match(TokenKind::KwI32)) {
    type = "i32";
    return true;
  }

  if (match(TokenKind::KwPtr)) {
    if (!expect(TokenKind::Less, "expected '<' after 'ptr'") ||
        !expect(TokenKind::KwI32, "expected pointee type 'i32'") ||
        !expect(TokenKind::Greater, "expected '>' after pointer type"))
      return false;
    type = "ptr<i32>";
    return true;
  }

  return false;
}

bool Parser::parseParameters(std::vector<ParameterAST> &parameters) {
  if (match(TokenKind::RParen))
    return true;

  while (true) {
    if (!check(TokenKind::Identifier)) {
      reportAtCurrent("expected parameter name");
      return false;
    }
    std::string name = advance().lexeme;

    if (!expect(TokenKind::Colon, "expected ':' after parameter name"))
      return false;

    std::string type;
    if (!parseType(type)) {
      reportAtCurrent("expected parameter type");
      return false;
    }
    parameters.emplace_back(std::move(name), std::move(type));

    if (match(TokenKind::RParen))
      return true;
    if (!expect(TokenKind::Comma, "expected ',' between parameters"))
      return false;
  }
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
  if (check(TokenKind::KwLet))
    return parseLetStatement();
  if (check(TokenKind::KwStore))
    return parseStoreStatement();
  if (check(TokenKind::KwPrintI32))
    return parsePrintI32Statement();
  if (check(TokenKind::KwVectorAdd))
    return parseVectorAddStatement();
  if (check(TokenKind::KwVectorCopy))
    return parseVectorCopyStatement();
  if (check(TokenKind::KwVectorScale))
    return parseVectorScaleStatement();
  if (check(TokenKind::KwVectorMul))
    return parseVectorMulStatement();
  if (check(TokenKind::KwVectorReduceAdd))
    return parseVectorReduceAddStatement();
  if (check(TokenKind::KwVectorSelectLT))
    return parseVectorSelectStatement(VectorSelectPredicate::LT,
                                      "vector_select_lt");
  if (check(TokenKind::KwVectorSelectLE))
    return parseVectorSelectStatement(VectorSelectPredicate::LE,
                                      "vector_select_le");
  if (check(TokenKind::KwVectorSelectGT))
    return parseVectorSelectStatement(VectorSelectPredicate::GT,
                                      "vector_select_gt");
  if (check(TokenKind::KwVectorSelectGE))
    return parseVectorSelectStatement(VectorSelectPredicate::GE,
                                      "vector_select_ge");
  if (check(TokenKind::KwVectorSelectEQ))
    return parseVectorSelectStatement(VectorSelectPredicate::EQ,
                                      "vector_select_eq");
  if (check(TokenKind::KwVectorSelectNE))
    return parseVectorSelectStatement(VectorSelectPredicate::NE,
                                      "vector_select_ne");
  if (check(TokenKind::KwVectorSelectULT))
    return parseVectorSelectStatement(VectorSelectPredicate::ULT,
                                      "vector_select_ult");
  if (check(TokenKind::KwVectorSelectULE))
    return parseVectorSelectStatement(VectorSelectPredicate::ULE,
                                      "vector_select_ule");
  if (check(TokenKind::KwVectorSelectUGT))
    return parseVectorSelectStatement(VectorSelectPredicate::UGT,
                                      "vector_select_ugt");
  if (check(TokenKind::KwVectorSelectUGE))
    return parseVectorSelectStatement(VectorSelectPredicate::UGE,
                                      "vector_select_uge");
  if (check(TokenKind::KwVectorMaskLT))
    return parseVectorMaskStatement(VectorSelectPredicate::LT,
                                    "vector_mask_lt");
  if (check(TokenKind::KwVectorMaskLE))
    return parseVectorMaskStatement(VectorSelectPredicate::LE,
                                    "vector_mask_le");
  if (check(TokenKind::KwVectorMaskGT))
    return parseVectorMaskStatement(VectorSelectPredicate::GT,
                                    "vector_mask_gt");
  if (check(TokenKind::KwVectorMaskGE))
    return parseVectorMaskStatement(VectorSelectPredicate::GE,
                                    "vector_mask_ge");
  if (check(TokenKind::KwVectorMaskEQ))
    return parseVectorMaskStatement(VectorSelectPredicate::EQ,
                                    "vector_mask_eq");
  if (check(TokenKind::KwVectorMaskNE))
    return parseVectorMaskStatement(VectorSelectPredicate::NE,
                                    "vector_mask_ne");
  if (check(TokenKind::KwVectorMaskULT))
    return parseVectorMaskStatement(VectorSelectPredicate::ULT,
                                    "vector_mask_ult");
  if (check(TokenKind::KwVectorMaskULE))
    return parseVectorMaskStatement(VectorSelectPredicate::ULE,
                                    "vector_mask_ule");
  if (check(TokenKind::KwVectorMaskUGT))
    return parseVectorMaskStatement(VectorSelectPredicate::UGT,
                                    "vector_mask_ugt");
  if (check(TokenKind::KwVectorMaskUGE))
    return parseVectorMaskStatement(VectorSelectPredicate::UGE,
                                    "vector_mask_uge");
  if (check(TokenKind::KwVectorMaskedAdd))
    return parseVectorMaskedBinaryStatement(VectorMaskedBinaryOp::Add,
                                            "vector_masked_add");
  if (check(TokenKind::KwVectorMaskedSub))
    return parseVectorMaskedBinaryStatement(VectorMaskedBinaryOp::Sub,
                                            "vector_masked_sub");
  if (check(TokenKind::KwVectorMaskedMul))
    return parseVectorMaskedBinaryStatement(VectorMaskedBinaryOp::Mul,
                                            "vector_masked_mul");
  if (check(TokenKind::KwVectorMaskedStore))
    return parseVectorMaskedStoreStatement();
  if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Equal)
    return parseAssignStatement();
  if (check(TokenKind::KwReturn))
    return parseReturnStatement();
  if (check(TokenKind::KwIf))
    return parseIfStatement();
  if (check(TokenKind::KwWhile))
    return parseWhileStatement();

  reportAtCurrent("expected statement");
  return nullptr;
}

std::unique_ptr<StmtAST> Parser::parseVectorAddStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected output buffer after 'vector_add'");
    return nullptr;
  }
  std::string output = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_add output"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected left input buffer in vector_add");
    return nullptr;
  }
  std::string lhs = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_add left input"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected right input buffer in vector_add");
    return nullptr;
  }
  std::string rhs = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_add right input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after vector_add statement"))
    return nullptr;

  return std::make_unique<VectorAddStmtAST>(
      std::move(output), std::move(lhs), std::move(rhs), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorCopyStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected output buffer after 'vector_copy'");
    return nullptr;
  }
  std::string output = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_copy output"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected input buffer in vector_copy");
    return nullptr;
  }
  std::string input = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_copy input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after vector_copy statement"))
    return nullptr;

  return std::make_unique<VectorCopyStmtAST>(
      std::move(output), std::move(input), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorScaleStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected output buffer after 'vector_scale'");
    return nullptr;
  }
  std::string output = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_scale output"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected input buffer in vector_scale");
    return nullptr;
  }
  std::string input = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_scale input"))
    return nullptr;

  auto factor = parseExpression();
  if (!factor)
    return nullptr;

  if (!expect(TokenKind::Comma, "expected ',' after vector_scale factor"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after vector_scale statement"))
    return nullptr;

  return std::make_unique<VectorScaleStmtAST>(
      std::move(output), std::move(input), std::move(factor),
      std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorMulStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected output buffer after 'vector_mul'");
    return nullptr;
  }
  std::string output = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_mul output"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected left input buffer in vector_mul");
    return nullptr;
  }
  std::string lhs = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_mul left input"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected right input buffer in vector_mul");
    return nullptr;
  }
  std::string rhs = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_mul right input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after vector_mul statement"))
    return nullptr;

  return std::make_unique<VectorMulStmtAST>(
      std::move(output), std::move(lhs), std::move(rhs), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorReduceAddStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected result variable after 'vector_reduce_add'");
    return nullptr;
  }
  std::string result = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_reduce_add result"))
    return nullptr;

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected input buffer in vector_reduce_add");
    return nullptr;
  }
  std::string input = advance().lexeme;

  if (!expect(TokenKind::Comma, "expected ',' after vector_reduce_add input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon,
              "expected ';' after vector_reduce_add statement"))
    return nullptr;

  return std::make_unique<VectorReduceAddStmtAST>(
      std::move(result), std::move(input), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorSelectStatement(
    VectorSelectPredicate predicate, StringRef keyword) {
  advance();

  auto parseBuffer = [this](StringRef diagnostic) -> std::string {
    if (!check(TokenKind::Identifier)) {
      reportAtCurrent(diagnostic);
      return {};
    }
    return advance().lexeme;
  };

  std::string output = parseBuffer("expected output buffer after vector_select");
  if (output.empty() ||
      !expect(TokenKind::Comma, "expected ',' after vector_select output"))
    return nullptr;

  std::string lhs = parseBuffer("expected left compare buffer in vector_select");
  if (lhs.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_select left compare input"))
    return nullptr;

  std::string rhs = parseBuffer("expected right compare buffer in vector_select");
  if (rhs.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_select right compare input"))
    return nullptr;

  std::string trueValues =
      parseBuffer("expected true-value buffer in vector_select");
  if (trueValues.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_select true-value input"))
    return nullptr;

  std::string falseValues =
      parseBuffer("expected false-value buffer in vector_select");
  if (falseValues.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_select false-value input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  std::string terminatorMessage =
      "expected ';' after " + keyword.str() + " statement";
  if (!expect(TokenKind::Semicolon, terminatorMessage))
    return nullptr;

  return std::make_unique<VectorSelectStmtAST>(
      predicate, std::move(output), std::move(lhs), std::move(rhs),
      std::move(trueValues), std::move(falseValues), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseStoreStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected buffer name after 'store'");
    return nullptr;
  }
  std::string bufferName = advance().lexeme;

  if (!expect(TokenKind::LBracket, "expected '[' before store index"))
    return nullptr;

  auto index = parseExpression();
  if (!index)
    return nullptr;

  if (!expect(TokenKind::RBracket, "expected ']' after store index") ||
      !expect(TokenKind::Equal, "expected '=' before store value"))
    return nullptr;

  auto value = parseExpression();
  if (!value)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after store statement"))
    return nullptr;

  return std::make_unique<StoreStmtAST>(std::move(bufferName),
                                        std::move(index), std::move(value));
}

std::unique_ptr<StmtAST> Parser::parsePrintI32Statement() {
  advance();

  auto value = parseExpression();
  if (!value)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after print_i32 statement"))
    return nullptr;

  return std::make_unique<PrintI32StmtAST>(std::move(value));
}

std::unique_ptr<StmtAST> Parser::parseAssignStatement() {
  std::string name = advance().lexeme;

  if (!expect(TokenKind::Equal, "expected '=' after assignment target"))
    return nullptr;

  auto value = parseExpression();
  if (!value)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after assignment"))
    return nullptr;

  return std::make_unique<AssignStmtAST>(std::move(name), std::move(value));
}

std::unique_ptr<StmtAST> Parser::parseLetStatement() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected variable name after 'let'");
    return nullptr;
  }
  std::string name = advance().lexeme;

  if (!expect(TokenKind::Equal, "expected '=' after variable name"))
    return nullptr;

  auto value = parseExpression();
  if (!value)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after let statement"))
    return nullptr;

  return std::make_unique<LetStmtAST>(std::move(name), std::move(value));
}


std::unique_ptr<StmtAST> Parser::parseVectorMaskStatement(
    VectorSelectPredicate predicate, StringRef keyword) {
  advance();

  auto parseName = [this](StringRef diagnostic) -> std::string {
    if (!check(TokenKind::Identifier)) {
      reportAtCurrent(diagnostic);
      return {};
    }
    return advance().lexeme;
  };

  std::string mask = parseName("expected mask name after vector_mask");
  if (mask.empty() || !expect(TokenKind::Comma, "expected ',' after mask name"))
    return nullptr;

  std::string lhs = parseName("expected left compare buffer in vector_mask");
  if (lhs.empty() || !expect(TokenKind::Comma, "expected ',' after vector_mask left input"))
    return nullptr;

  std::string rhs = parseName("expected right compare buffer in vector_mask");
  if (rhs.empty() || !expect(TokenKind::Comma, "expected ',' after vector_mask right input"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  std::string terminatorMessage =
      "expected ';' after " + keyword.str() + " statement";
  if (!expect(TokenKind::Semicolon, terminatorMessage))
    return nullptr;

  return std::make_unique<VectorMaskStmtAST>(predicate, std::move(mask),
                                             std::move(lhs), std::move(rhs),
                                             std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseVectorMaskedBinaryStatement(
    VectorMaskedBinaryOp op, StringRef keyword) {
  advance();

  auto parseName = [this](StringRef diagnostic) -> std::string {
    if (!check(TokenKind::Identifier)) {
      reportAtCurrent(diagnostic);
      return {};
    }
    return advance().lexeme;
  };

  std::string output =
      parseName("expected output buffer after " + keyword.str());
  if (output.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after " + keyword.str() + " output"))
    return nullptr;

  std::string lhs = parseName("expected left input buffer in " + keyword.str());
  if (lhs.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after " + keyword.str() + " left input"))
    return nullptr;

  std::string rhs =
      parseName("expected right input buffer in " + keyword.str());
  if (rhs.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after " + keyword.str() + " right input"))
    return nullptr;

  std::string mask = parseName("expected mask name in " + keyword.str());
  if (mask.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after " + keyword.str() + " mask"))
    return nullptr;

  std::string passthrough =
      parseName("expected passthrough buffer in " + keyword.str());
  if (passthrough.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after " + keyword.str() + " passthrough"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon,
              "expected ';' after " + keyword.str() + " statement"))
    return nullptr;

  return std::make_unique<VectorMaskedBinaryStmtAST>(
      op, std::move(output), std::move(lhs), std::move(rhs), std::move(mask),
      std::move(passthrough), std::move(length));
}


std::unique_ptr<StmtAST> Parser::parseVectorMaskedStoreStatement() {
  advance();

  auto parseName = [this](StringRef diagnostic) -> std::string {
    if (!check(TokenKind::Identifier)) {
      reportAtCurrent(diagnostic);
      return {};
    }
    return advance().lexeme;
  };

  std::string output =
      parseName("expected output buffer after vector_masked_store");
  if (output.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_masked_store output"))
    return nullptr;

  std::string input = parseName("expected input buffer in vector_masked_store");
  if (input.empty() ||
      !expect(TokenKind::Comma,
              "expected ',' after vector_masked_store input"))
    return nullptr;

  std::string mask = parseName("expected mask name in vector_masked_store");
  if (mask.empty() ||
      !expect(TokenKind::Comma, "expected ',' after vector_masked_store mask"))
    return nullptr;

  auto length = parseExpression();
  if (!length)
    return nullptr;

  if (!expect(TokenKind::Semicolon,
              "expected ';' after vector_masked_store statement"))
    return nullptr;

  return std::make_unique<VectorMaskedStoreStmtAST>(
      std::move(output), std::move(input), std::move(mask), std::move(length));
}

std::unique_ptr<StmtAST> Parser::parseReturnStatement() {
  advance();

  auto value = parseExpression();
  if (!value)
    return nullptr;

  if (!expect(TokenKind::Semicolon, "expected ';' after return statement"))
    return nullptr;

  return std::make_unique<ReturnStmtAST>(std::move(value));
}

std::unique_ptr<StmtAST> Parser::parseIfStatement() {
  advance();

  auto condition = parseExpression();
  if (!condition)
    return nullptr;

  if (!expect(TokenKind::LBrace, "expected '{' before if body"))
    return nullptr;

  std::vector<std::unique_ptr<StmtAST>> thenBody;
  if (!parseBlock(thenBody))
    return nullptr;

  std::vector<std::unique_ptr<StmtAST>> elseBody;
  if (match(TokenKind::KwElse)) {
    if (!expect(TokenKind::LBrace, "expected '{' before else body"))
      return nullptr;
    if (!parseBlock(elseBody))
      return nullptr;
  }

  return std::make_unique<IfStmtAST>(std::move(condition), std::move(thenBody),
                                     std::move(elseBody));
}

std::unique_ptr<StmtAST> Parser::parseWhileStatement() {
  advance();

  auto condition = parseExpression();
  if (!condition)
    return nullptr;

  if (!expect(TokenKind::LBrace, "expected '{' before while body"))
    return nullptr;

  std::vector<std::unique_ptr<StmtAST>> body;
  if (!parseBlock(body))
    return nullptr;

  return std::make_unique<WhileStmtAST>(std::move(condition), std::move(body));
}

bool Parser::parseBlock(std::vector<std::unique_ptr<StmtAST>> &statements) {
  while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
    auto statement = parseStatement();
    if (!statement)
      return false;
    statements.push_back(std::move(statement));
  }

  return expect(TokenKind::RBrace, "expected '}' after block");
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
  auto lhs = parsePrimary();
  if (!lhs)
    return nullptr;
  return parseBinaryRHS(0, std::move(lhs));
}

std::unique_ptr<ExprAST>
Parser::parseBinaryRHS(int expressionPrecedence, std::unique_ptr<ExprAST> lhs) {
  while (true) {
    int tokenPrecedence = getTokenPrecedence();
    if (tokenPrecedence < expressionPrecedence)
      return lhs;

    std::string op = advance().lexeme;
    auto rhs = parsePrimary();
    if (!rhs)
      return nullptr;

    int nextPrecedence = getTokenPrecedence();
    if (tokenPrecedence < nextPrecedence) {
      rhs = parseBinaryRHS(tokenPrecedence + 1, std::move(rhs));
      if (!rhs)
        return nullptr;
    }

    lhs = std::make_unique<BinaryExprAST>(std::move(op), std::move(lhs),
                                          std::move(rhs));
  }
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
  if (check(TokenKind::Integer))
    return std::make_unique<IntegerExprAST>(advance().lexeme);

  if (check(TokenKind::KwLoad))
    return parseLoadExpression();

  if (check(TokenKind::Identifier))
    return parseIdentifierExpression();

  if (match(TokenKind::LParen)) {
    auto expression = parseExpression();
    if (!expression)
      return nullptr;
    if (!expect(TokenKind::RParen, "expected ')' after expression"))
      return nullptr;
    return expression;
  }

  reportAtCurrent("expected expression");
  return nullptr;
}

std::unique_ptr<ExprAST> Parser::parseLoadExpression() {
  advance();

  if (!check(TokenKind::Identifier)) {
    reportAtCurrent("expected buffer name after 'load'");
    return nullptr;
  }
  std::string bufferName = advance().lexeme;

  if (!expect(TokenKind::LBracket, "expected '[' before load index"))
    return nullptr;

  auto index = parseExpression();
  if (!index)
    return nullptr;

  if (!expect(TokenKind::RBracket, "expected ']' after load index"))
    return nullptr;

  return std::make_unique<LoadExprAST>(std::move(bufferName),
                                       std::move(index));
}

std::unique_ptr<ExprAST> Parser::parseIdentifierExpression() {
  std::string name = advance().lexeme;

  if (!match(TokenKind::LParen))
    return std::make_unique<VariableExprAST>(std::move(name));

  std::vector<std::unique_ptr<ExprAST>> args;
  if (!parseArguments(args))
    return nullptr;
  return std::make_unique<CallExprAST>(std::move(name), std::move(args));
}

bool Parser::parseArguments(std::vector<std::unique_ptr<ExprAST>> &args) {
  if (match(TokenKind::RParen))
    return true;

  while (true) {
    auto arg = parseExpression();
    if (!arg)
      return false;
    args.push_back(std::move(arg));

    if (match(TokenKind::RParen))
      return true;
    if (!expect(TokenKind::Comma, "expected ',' between arguments"))
      return false;
  }
}

int Parser::getTokenPrecedence() const {
  switch (peek().kind) {
  case TokenKind::Plus:
  case TokenKind::Minus:
    return 10;
  case TokenKind::Star:
  case TokenKind::Slash:
    return 20;
  case TokenKind::Less:
  case TokenKind::LessEqual:
  case TokenKind::Greater:
  case TokenKind::GreaterEqual:
  case TokenKind::EqualEqual:
  case TokenKind::BangEqual:
    return 5;
  default:
    return -1;
  }
}

} // namespace zc
