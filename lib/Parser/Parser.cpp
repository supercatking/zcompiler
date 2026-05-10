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

  if (!expect(TokenKind::LParen, "expected '(' after function name") ||
      !expect(TokenKind::RParen, "expected ')' after function parameters") ||
      !expect(TokenKind::Arrow, "expected '->' before return type"))
    return nullptr;

  if (!check(TokenKind::KwI32)) {
    reportAtCurrent("expected return type 'i32'");
    return nullptr;
  }
  std::string returnType = advance().lexeme;

  if (!expect(TokenKind::LBrace, "expected '{' before function body"))
    return nullptr;

  std::vector<std::unique_ptr<StmtAST>> body;
  if (!parseBlock(body))
    return nullptr;

  return std::make_unique<FunctionAST>(std::move(name), std::move(returnType),
                                       std::move(body));
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
  if (check(TokenKind::KwLet))
    return parseLetStatement();
  if (check(TokenKind::KwReturn))
    return parseReturnStatement();
  if (check(TokenKind::KwIf))
    return parseIfStatement();
  if (check(TokenKind::KwWhile))
    return parseWhileStatement();

  reportAtCurrent("expected statement");
  return nullptr;
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

  if (check(TokenKind::Identifier))
    return std::make_unique<VariableExprAST>(advance().lexeme);

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
