#include "zcompiler/Lexer/Lexer.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/FormatVariadic.h"

#include <cctype>

using namespace llvm;

namespace zc {

Lexer::Lexer(StringRef source) : source(source) {}

std::vector<Token> Lexer::lexAll() {
  std::vector<Token> tokens;

  while (true) {
    skipWhitespaceAndComments();

    unsigned tokenLine = line;
    unsigned tokenColumn = column;
    unsigned start = current;

    if (isAtEnd()) {
      tokens.push_back({TokenKind::EndOfFile, "", tokenLine, tokenColumn});
      return tokens;
    }

    char value = advance();
    switch (value) {
    case '+':
      tokens.push_back(makeToken(TokenKind::Plus, start, tokenLine, tokenColumn));
      break;
    case '-':
      if (peek() == '>') {
        advance();
        tokens.push_back(
            makeToken(TokenKind::Arrow, start, tokenLine, tokenColumn));
      } else {
        tokens.push_back(
            makeToken(TokenKind::Minus, start, tokenLine, tokenColumn));
      }
      break;
    case '*':
      tokens.push_back(makeToken(TokenKind::Star, start, tokenLine, tokenColumn));
      break;
    case '/':
      tokens.push_back(makeToken(TokenKind::Slash, start, tokenLine, tokenColumn));
      break;
    case '=':
      if (peek() == '=') {
        advance();
        tokens.push_back(
            makeToken(TokenKind::EqualEqual, start, tokenLine, tokenColumn));
      } else {
        tokens.push_back(
            makeToken(TokenKind::Equal, start, tokenLine, tokenColumn));
      }
      break;
    case '!':
      if (peek() == '=') {
        advance();
        tokens.push_back(
            makeToken(TokenKind::BangEqual, start, tokenLine, tokenColumn));
      } else {
        reportInvalidCharacter(value, tokenLine, tokenColumn);
      }
      break;
    case '<':
      if (peek() == '=') {
        advance();
        tokens.push_back(
            makeToken(TokenKind::LessEqual, start, tokenLine, tokenColumn));
      } else {
        tokens.push_back(
            makeToken(TokenKind::Less, start, tokenLine, tokenColumn));
      }
      break;
    case '>':
      if (peek() == '=') {
        advance();
        tokens.push_back(
            makeToken(TokenKind::GreaterEqual, start, tokenLine, tokenColumn));
      } else {
        tokens.push_back(
            makeToken(TokenKind::Greater, start, tokenLine, tokenColumn));
      }
      break;
    case '(':
      tokens.push_back(makeToken(TokenKind::LParen, start, tokenLine, tokenColumn));
      break;
    case ')':
      tokens.push_back(makeToken(TokenKind::RParen, start, tokenLine, tokenColumn));
      break;
    case '{':
      tokens.push_back(makeToken(TokenKind::LBrace, start, tokenLine, tokenColumn));
      break;
    case '}':
      tokens.push_back(makeToken(TokenKind::RBrace, start, tokenLine, tokenColumn));
      break;
    case '[':
      tokens.push_back(
          makeToken(TokenKind::LBracket, start, tokenLine, tokenColumn));
      break;
    case ']':
      tokens.push_back(
          makeToken(TokenKind::RBracket, start, tokenLine, tokenColumn));
      break;
    case ':':
      tokens.push_back(makeToken(TokenKind::Colon, start, tokenLine, tokenColumn));
      break;
    case ',':
      tokens.push_back(makeToken(TokenKind::Comma, start, tokenLine, tokenColumn));
      break;
    case ';':
      tokens.push_back(
          makeToken(TokenKind::Semicolon, start, tokenLine, tokenColumn));
      break;
    default:
      if (std::isalpha(static_cast<unsigned char>(value)) || value == '_') {
        current = start;
        line = tokenLine;
        column = tokenColumn;
        tokens.push_back(lexIdentifierOrKeyword());
      } else if (std::isdigit(static_cast<unsigned char>(value))) {
        current = start;
        line = tokenLine;
        column = tokenColumn;
        tokens.push_back(lexInteger());
      } else {
        reportInvalidCharacter(value, tokenLine, tokenColumn);
      }
      break;
    }
  }
}

char Lexer::peek(unsigned offset) const {
  unsigned index = current + offset;
  if (index >= source.size())
    return '\0';
  return source[index];
}

char Lexer::advance() {
  char value = peek();
  if (value == '\0')
    return value;

  ++current;
  if (value == '\n') {
    ++line;
    column = 1;
  } else {
    ++column;
  }
  return value;
}

bool Lexer::isAtEnd() const { return current >= source.size(); }

void Lexer::skipWhitespaceAndComments() {
  while (!isAtEnd()) {
    char value = peek();

    if (value == ' ' || value == '\t' || value == '\r' || value == '\n') {
      advance();
      continue;
    }

    if (value == '/' && peek(1) == '/') {
      while (!isAtEnd() && peek() != '\n')
        advance();
      continue;
    }

    return;
  }
}

Token Lexer::lexIdentifierOrKeyword() {
  unsigned start = current;
  unsigned tokenLine = line;
  unsigned tokenColumn = column;

  while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
    advance();

  StringRef text = source.slice(start, current);
  TokenKind kind = TokenKind::Identifier;
  if (text == "func")
    kind = TokenKind::KwFunc;
  else if (text == "let")
    kind = TokenKind::KwLet;
  else if (text == "return")
    kind = TokenKind::KwReturn;
  else if (text == "if")
    kind = TokenKind::KwIf;
  else if (text == "else")
    kind = TokenKind::KwElse;
  else if (text == "while")
    kind = TokenKind::KwWhile;
  else if (text == "i32")
    kind = TokenKind::KwI32;
  else if (text == "ptr")
    kind = TokenKind::KwPtr;
  else if (text == "load")
    kind = TokenKind::KwLoad;
  else if (text == "store")
    kind = TokenKind::KwStore;
  else if (text == "print_i32")
    kind = TokenKind::KwPrintI32;
  else if (text == "vector_add")
    kind = TokenKind::KwVectorAdd;
  else if (text == "vector_copy")
    kind = TokenKind::KwVectorCopy;
  else if (text == "vector_scale")
    kind = TokenKind::KwVectorScale;
  else if (text == "vector_mul")
    kind = TokenKind::KwVectorMul;
  else if (text == "vector_reduce_add")
    kind = TokenKind::KwVectorReduceAdd;
  else if (text == "vector_select_lt")
    kind = TokenKind::KwVectorSelectLT;
  else if (text == "vector_select_le")
    kind = TokenKind::KwVectorSelectLE;
  else if (text == "vector_select_gt")
    kind = TokenKind::KwVectorSelectGT;
  else if (text == "vector_select_ge")
    kind = TokenKind::KwVectorSelectGE;
  else if (text == "vector_select_eq")
    kind = TokenKind::KwVectorSelectEQ;
  else if (text == "vector_select_ne")
    kind = TokenKind::KwVectorSelectNE;
  else if (text == "vector_select_ult")
    kind = TokenKind::KwVectorSelectULT;
  else if (text == "vector_select_ule")
    kind = TokenKind::KwVectorSelectULE;
  else if (text == "vector_select_ugt")
    kind = TokenKind::KwVectorSelectUGT;
  else if (text == "vector_select_uge")
    kind = TokenKind::KwVectorSelectUGE;
  else if (text == "vector_mask_gt")
    kind = TokenKind::KwVectorMaskGT;
  else if (text == "vector_masked_add")
    kind = TokenKind::KwVectorMaskedAdd;

  return {kind, text.str(), tokenLine, tokenColumn};
}

Token Lexer::lexInteger() {
  unsigned start = current;
  unsigned tokenLine = line;
  unsigned tokenColumn = column;

  while (std::isdigit(static_cast<unsigned char>(peek())))
    advance();

  return makeToken(TokenKind::Integer, start, tokenLine, tokenColumn);
}

Token Lexer::makeToken(TokenKind kind, unsigned start, unsigned tokenLine,
                       unsigned tokenColumn) const {
  return {kind, source.slice(start, current).str(), tokenLine, tokenColumn};
}

void Lexer::reportInvalidCharacter(char value, unsigned errorLine,
                                   unsigned errorColumn) {
  diagnostics.push_back(
      formatv("{0}:{1}: invalid character '{2}'", errorLine, errorColumn, value)
          .str());
}

void printTokens(ArrayRef<Token> tokens, raw_ostream &os) {
  for (const Token &token : tokens) {
    os << token.line << ':' << token.column << ' '
       << getTokenKindName(token.kind);
    if (!token.lexeme.empty())
      os << " \"" << token.lexeme << '"';
    os << '\n';
  }
}

} // namespace zc
