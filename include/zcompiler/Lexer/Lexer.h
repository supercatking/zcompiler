#ifndef ZCOMPILER_LEXER_LEXER_H
#define ZCOMPILER_LEXER_LEXER_H

#include "zcompiler/Lexer/Token.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

namespace zc {

class Lexer {
public:
  explicit Lexer(llvm::StringRef source);

  std::vector<Token> lexAll();
  bool hasError() const { return !diagnostics.empty(); }
  const std::vector<std::string> &getDiagnostics() const { return diagnostics; }

private:
  char peek(unsigned offset = 0) const;
  char advance();
  bool isAtEnd() const;

  void skipWhitespaceAndComments();
  Token lexIdentifierOrKeyword();
  Token lexInteger();
  Token makeToken(TokenKind kind, unsigned start, unsigned tokenLine,
                  unsigned tokenColumn) const;
  void reportInvalidCharacter(char value, unsigned errorLine,
                              unsigned errorColumn);

  llvm::StringRef source;
  unsigned current = 0;
  unsigned line = 1;
  unsigned column = 1;
  std::vector<std::string> diagnostics;
};

void printTokens(llvm::ArrayRef<Token> tokens, llvm::raw_ostream &os);

} // namespace zc

#endif // ZCOMPILER_LEXER_LEXER_H

