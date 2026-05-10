#include "zcompiler/Lexer/Token.h"

using namespace llvm;

namespace zc {

StringRef getTokenKindName(TokenKind kind) {
  switch (kind) {
  case TokenKind::EndOfFile:
    return "eof";
  case TokenKind::Identifier:
    return "identifier";
  case TokenKind::Integer:
    return "integer";
  case TokenKind::KwFunc:
    return "kw_func";
  case TokenKind::KwLet:
    return "kw_let";
  case TokenKind::KwReturn:
    return "kw_return";
  case TokenKind::KwI32:
    return "kw_i32";
  case TokenKind::Plus:
    return "plus";
  case TokenKind::Minus:
    return "minus";
  case TokenKind::Star:
    return "star";
  case TokenKind::Slash:
    return "slash";
  case TokenKind::Equal:
    return "equal";
  case TokenKind::Arrow:
    return "arrow";
  case TokenKind::LParen:
    return "l_paren";
  case TokenKind::RParen:
    return "r_paren";
  case TokenKind::LBrace:
    return "l_brace";
  case TokenKind::RBrace:
    return "r_brace";
  case TokenKind::Semicolon:
    return "semicolon";
  }
  return "unknown";
}

} // namespace zc

