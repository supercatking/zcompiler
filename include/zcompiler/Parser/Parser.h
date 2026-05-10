#ifndef ZCOMPILER_PARSER_PARSER_H
#define ZCOMPILER_PARSER_PARSER_H

#include "zcompiler/AST/AST.h"
#include "zcompiler/Lexer/Token.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include <memory>
#include <string>
#include <vector>

namespace zc {

class Parser {
public:
  explicit Parser(llvm::ArrayRef<Token> tokens);

  std::unique_ptr<ModuleAST> parseModule();
  bool hasError() const { return !diagnostics.empty(); }
  const std::vector<std::string> &getDiagnostics() const { return diagnostics; }

private:
  const Token &peek(unsigned offset = 0) const;
  const Token &advance();
  bool check(TokenKind kind) const;
  bool match(TokenKind kind);
  bool expect(TokenKind kind, llvm::StringRef message);
  void reportAtCurrent(llvm::StringRef message);

  std::unique_ptr<FunctionAST> parseFunction();
  std::unique_ptr<StmtAST> parseStatement();
  std::unique_ptr<StmtAST> parseLetStatement();
  std::unique_ptr<StmtAST> parseReturnStatement();
  std::unique_ptr<StmtAST> parseIfStatement();
  std::unique_ptr<StmtAST> parseWhileStatement();
  bool parseBlock(std::vector<std::unique_ptr<StmtAST>> &statements);

  std::unique_ptr<ExprAST> parseExpression();
  std::unique_ptr<ExprAST> parseBinaryRHS(int expressionPrecedence,
                                          std::unique_ptr<ExprAST> lhs);
  std::unique_ptr<ExprAST> parsePrimary();
  int getTokenPrecedence() const;

  llvm::ArrayRef<Token> tokens;
  unsigned current = 0;
  std::vector<std::string> diagnostics;
};

} // namespace zc

#endif // ZCOMPILER_PARSER_PARSER_H
