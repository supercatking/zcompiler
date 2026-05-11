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
