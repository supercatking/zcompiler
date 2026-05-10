#ifndef ZCOMPILER_LEXER_TOKEN_H
#define ZCOMPILER_LEXER_TOKEN_H

#include "llvm/ADT/StringRef.h"

#include <string>

namespace zc {

enum class TokenKind {
  EndOfFile,
  Identifier,
  Integer,

  KwFunc,
  KwLet,
  KwReturn,
  KwI32,

  Plus,
  Minus,
  Star,
  Slash,
  Equal,
  Arrow,

  LParen,
  RParen,
  LBrace,
  RBrace,
  Semicolon,
};

struct Token {
  TokenKind kind;
  std::string lexeme;
  unsigned line;
  unsigned column;
};

llvm::StringRef getTokenKindName(TokenKind kind);

} // namespace zc

#endif // ZCOMPILER_LEXER_TOKEN_H

