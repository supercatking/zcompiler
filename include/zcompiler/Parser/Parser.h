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
  bool parseType(std::string &type);
  bool parseParameters(std::vector<ParameterAST> &parameters);
  std::unique_ptr<StmtAST> parseStatement();
  std::unique_ptr<StmtAST> parseLetStatement();
  std::unique_ptr<StmtAST> parseAssignStatement();
  std::unique_ptr<StmtAST> parseStoreStatement();
  std::unique_ptr<StmtAST> parsePrintI32Statement();
  std::unique_ptr<StmtAST> parseMatrixPackBStatement();
  std::unique_ptr<StmtAST> parseMatrixMultiplyStatement();
  std::unique_ptr<StmtAST> parseVectorAddStatement();
  std::unique_ptr<StmtAST> parseVectorStridedLoadStatement();
  std::unique_ptr<StmtAST> parseVectorIndexedLoadStatement();
  std::unique_ptr<StmtAST> parseVectorStridedStoreStatement();
  std::unique_ptr<StmtAST> parseVectorIndexedStoreStatement();
  std::unique_ptr<StmtAST> parseVectorCopyStatement();
  std::unique_ptr<StmtAST> parseVectorScaleStatement();
  std::unique_ptr<StmtAST> parseVectorMulStatement();
  std::unique_ptr<StmtAST> parseVectorWidenAddI16I32Statement();
  std::unique_ptr<StmtAST> parseVectorReduceAddStatement();
  std::unique_ptr<StmtAST>
  parseVectorSelectStatement(VectorSelectPredicate predicate,
                             llvm::StringRef keyword);
  std::unique_ptr<StmtAST>
  parseVectorMaskStatement(VectorSelectPredicate predicate,
                           llvm::StringRef keyword);
  std::unique_ptr<StmtAST>
  parseVectorMaskLogicalStatement(VectorMaskLogicalOp op,
                                  llvm::StringRef keyword);
  std::unique_ptr<StmtAST>
  parseVectorMaskedBinaryStatement(VectorMaskedBinaryOp op,
                                   llvm::StringRef keyword);
  std::unique_ptr<StmtAST> parseVectorMaskedStoreStatement();
  std::unique_ptr<StmtAST> parseVectorMaskedLoadStatement();
  std::unique_ptr<StmtAST> parseVectorMaskedStridedLoadStatement();
  std::unique_ptr<StmtAST> parseVectorMaskedIndexedLoadStatement();
  std::unique_ptr<StmtAST> parseVectorMaskedStridedStoreStatement();
  std::unique_ptr<StmtAST> parseVectorMaskedIndexedStoreStatement();
  std::unique_ptr<StmtAST> parseReturnStatement();
  std::unique_ptr<StmtAST> parseIfStatement();
  std::unique_ptr<StmtAST> parseWhileStatement();
  bool parseBlock(std::vector<std::unique_ptr<StmtAST>> &statements);

  std::unique_ptr<ExprAST> parseExpression();
  std::unique_ptr<ExprAST> parseBinaryRHS(int expressionPrecedence,
                                          std::unique_ptr<ExprAST> lhs);
  std::unique_ptr<ExprAST> parsePrimary();
  std::unique_ptr<ExprAST> parseIdentifierExpression();
  std::unique_ptr<ExprAST> parseLoadExpression();
  bool parseArguments(std::vector<std::unique_ptr<ExprAST>> &args);
  int getTokenPrecedence() const;

  llvm::ArrayRef<Token> tokens;
  unsigned current = 0;
  std::vector<std::string> diagnostics;
};

} // namespace zc

#endif // ZCOMPILER_PARSER_PARSER_H
