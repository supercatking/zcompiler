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
  case TokenKind::KwIf:
    return "kw_if";
  case TokenKind::KwElse:
    return "kw_else";
  case TokenKind::KwWhile:
    return "kw_while";
  case TokenKind::KwI32:
    return "kw_i32";
  case TokenKind::KwPtr:
    return "kw_ptr";
  case TokenKind::KwLoad:
    return "kw_load";
  case TokenKind::KwStore:
    return "kw_store";
  case TokenKind::KwVectorAdd:
    return "kw_vector_add";
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
  case TokenKind::EqualEqual:
    return "equal_equal";
  case TokenKind::BangEqual:
    return "bang_equal";
  case TokenKind::Less:
    return "less";
  case TokenKind::LessEqual:
    return "less_equal";
  case TokenKind::Greater:
    return "greater";
  case TokenKind::GreaterEqual:
    return "greater_equal";
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
  case TokenKind::LBracket:
    return "l_bracket";
  case TokenKind::RBracket:
    return "r_bracket";
  case TokenKind::Colon:
    return "colon";
  case TokenKind::Comma:
    return "comma";
  case TokenKind::Semicolon:
    return "semicolon";
  }
  return "unknown";
}

} // namespace zc
