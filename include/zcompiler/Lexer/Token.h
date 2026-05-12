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
  KwI8,
  KwI16,
  KwI32,
  KwI64,
  KwPtr,
  KwLoad,
  KwStore,
  KwPrintI32,
  KwMatrixPackB,
  KwMatrixMultiply,
  KwMatrixMultiplyPackedB,
  KwVectorAdd,
  KwVectorAddM2,
  KwVectorAddM4,
  KwVectorStridedLoad,
  KwVectorIndexedLoad,
  KwVectorCopy,
  KwVectorScale,
  KwVectorMul,
  KwVectorWidenAddI16I32,
  KwVectorReduceAdd,
  KwVectorSelectLT,
  KwVectorSelectLE,
  KwVectorSelectGT,
  KwVectorSelectGE,
  KwVectorSelectEQ,
  KwVectorSelectNE,
  KwVectorSelectULT,
  KwVectorSelectULE,
  KwVectorSelectUGT,
  KwVectorSelectUGE,
  KwVectorMaskLT,
  KwVectorMaskLE,
  KwVectorMaskGT,
  KwVectorMaskGE,
  KwVectorMaskEQ,
  KwVectorMaskNE,
  KwVectorMaskULT,
  KwVectorMaskULE,
  KwVectorMaskUGT,
  KwVectorMaskUGE,
  KwVectorMaskAnd,
  KwVectorMaskOr,
  KwVectorMaskXor,
  KwVectorMaskNot,
  KwVectorMaskedAdd,
  KwVectorMaskedSub,
  KwVectorMaskedMul,
  KwVectorMaskedStore,
  KwVectorMaskedLoad,

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
