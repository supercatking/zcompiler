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
  KwIf,
  KwElse,
  KwWhile,
  KwI32,
  KwPtr,
  KwLoad,
  KwStore,
  KwVectorAdd,

  Plus,
  Minus,
  Star,
  Slash,
  Equal,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Arrow,

  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Colon,
  Comma,
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
