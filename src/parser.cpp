#include "parser.h"
#include <charconv>
#include <iostream>

bool is_digit(const char &c) { return '0' <= c && c <= '9'; }

bool is_ws(const char &c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\f' || c == '\v' ||
         c == '\r';
}

bool Parser::expect(const std::function<bool(char)> &&fn) {
  if (iter == input.end())
    return false;
  if (!fn(*iter))
    return false;
  return true;
}

bool Parser::expect(const char &&c) {
  if (iter == input.end())
    return false;
  if (c != *iter)
    return false;
  return true;
}

bool Parser::expect(const std::string &&str) {
  auto cursor = iter;
  for (auto &c : str) {
    if (cursor == input.end() || c != *cursor) {
      return false;
    }
    ++cursor;
  }
  return true;
}

Expr Parser::parse_number() {
  trim();

  if (!expect(is_digit))
    throw ParserError::FailTry;

  auto start = iter;
  while (expect(is_digit)) {
    ++iter;
  }

  if (expect('.')) {
    ++iter;
    while (expect(is_digit)) {
      ++iter;
    }
  }

  std::string_view sw{start, iter};
  float val{};
  auto _ = std::from_chars(sw.data(), sw.data() + sw.size(), val);

  return ExprNumber{.value = val};
}

Expr Parser::parse_string() {
  trim();

  if (!expect('"'))
    throw ParserError::FailTry;

  ++iter;

  auto start = iter;
  while (!expect('"')) {
    ++iter;
  }

  if (iter >= input.end()) {
    throw ParserError::UnclosedQuote;
  }

  std::string str(start, iter);

  ++iter;

  return ExprString{.value = str};
}

Expr Parser::parse_op0() {
  auto left = parse_op1();
  trim();

  while (1) {
    Operator op;
    if (expect('+')) {
      op = Operator::Add;
    } else if (expect('-')) {
      op = Operator::Sub;
    } else {
      break;
    }
    ++iter;
    auto right = parse_op1();
    left = ExprOperation{.left = std::make_shared<Expr>(left),
                         .right = std::make_shared<Expr>(right),
                         .op = op};
    trim();
  }

  return left;
}

Expr Parser::parse_op1() {
  auto left = parse_atom();
  trim();

  while (1) {
    Operator op;
    if (expect('*')) {
      op = Operator::Mul;
    } else if (expect('/')) {
      op = Operator::Div;
    } else {
      break;
    }
    ++iter;
    auto right = parse_atom();
    left = ExprOperation{.left = std::make_shared<Expr>(left),
                         .right = std::make_shared<Expr>(right),
                         .op = op};
    trim();
  }

  return left;
}

Expr Parser::parse_operation() {
  trim();

  try {
    return parse_op0();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  try {
    return parse_op1();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  throw ParserError::FailTry;
}

Expr Parser::parse_atom() {
  trim();

  try {
    return parse_number();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  try {
    return parse_match();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  try {
    return parse_fncall(); // Falls through to symbol
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  // try {
  //   return parse_symbol();
  // } catch (ParserError &e) {
  //   if (e != ParserError::FailTry)
  //     throw e;
  // }

  try {
    return parse_string();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  throw ParserError::FailTry;
}

Expr Parser::parse_expr() {
  trim();

  try {
    return parse_fndecl();
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  try {
    return parse_operation(); // falls through to atom anyways
  } catch (ParserError &e) {
    if (e != ParserError::FailTry)
      throw e;
  }

  // try {
  //   return parse_atom();
  // } catch (ParserError &e) {
  //   if (e != ParserError::FailTry)
  //     throw e;
  // }

  throw ParserError::UnknownExpression;
}

void Parser::trim() {
  while (iter != input.end()) {
    if (is_ws(*iter)) {
      ++iter;
    } else {
      break;
    }
  }
}

void Parser::parse_all() {
  while (iter != input.end()) {
    output.push_back(parse_expr());
  }
}

void print_binop(ExprOperation op) {
  switch (op.op) {
  case Operator::Add:
    std::cout << "(+ ";
    break;
  case Operator::Sub:
    std::cout << "(- ";
    break;
  case Operator::Mul:
    std::cout << "(* ";
    break;
  case Operator::Div:
    std::cout << "(/ ";
    break;
  }
  std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ExprOperation>) {
          print_binop(arg);
        } else {
          print_expr(arg);
        }
      },
      *op.left);
  std::cout << " ";
  std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ExprOperation>) {
          print_binop(arg);
        } else {
          print_expr(arg);
        }
      },
      *op.right);
  std::cout << ")";
}

void print_expr(const Expr &e) {
  std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ExprNumber>) {
          std::cout << arg.value;
        } else if constexpr (std::is_same_v<T, ExprString>) {
          std::cout << '"' << arg.value << '"';
        } else if constexpr (std::is_same_v<T, ExprSymbol>) {
          std::cout << arg.value;
        } else if constexpr (std::is_same_v<T, ExprFnCall>) {
          std::cout << "(" << arg.name;
          for (auto e : arg.args) {
            std::cout << " ";
            print_expr(e);
          }
          std::cout << ")";
        } else if constexpr (std::is_same_v<T, ExprFnDecl>) {
          std::cout << "(func " << arg.name << "(";
          std::string args{};
          for (auto &e : arg.args) {
            args.append(e.first);
            args.append(": ");
            args.append(e.second);
            args.append(", ");
          }
          if (arg.args.size() > 0) {
            args.pop_back();
            args.pop_back();
          }
          std::cout << args << ") -> " << arg.ret_type << " ";
          print_expr(*arg.body);
          std::cout << ")";
        } else if constexpr (std::is_same_v<T, ExprOperation>) {
          print_binop(arg);
        } else if constexpr (std::is_same_v<T, ExprMatch>) {
          std::cout << "(match ";
          print_expr(*arg.value);
          for (auto branch : arg.branches) {
            std::cout << " (";
            print_expr(*branch.pattern);
            std::cout << " -> ";
            print_expr(*branch.value);
            std::cout << ")";
          }
          std::cout << ")";
        }
      },
      e);
}

bool is_symbol(const char &&c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') || c == '_';
}

Expr Parser::parse_symbol() {
  if (!expect(is_symbol))
    throw ParserError::FailTry;

  auto start = iter;
  while (expect(is_symbol)) {
    ++iter;
  }

  std::string str(start, iter);

  return ExprSymbol{.value = str};
}

std::string::iterator Parser::find_braces(const char &&left,
                                          const char &&right) {
  auto rparen = iter;
  int depth = 0;
  while (rparen != input.end()) {
    if (*rparen == left)
      ++depth;
    if (*rparen == right) {
      --depth;
      if (depth == 0)
        break;
    }
    ++rparen;
  }
  return rparen;
}

Expr Parser::parse_fncall() {
  auto sym = parse_symbol();
  trim();
  if (!expect('(')) {
    return sym;
  }
  auto close = find_braces('(', ')');
  if (close == input.end()) {
    throw ParserError::UnclosedParens;
  }
  ++iter;
  std::vector<Expr> args{};
  args.reserve(10);
  while (iter != close) {
    trim();
    args.push_back(parse_expr());
    trim();
    if (!expect(',')) {
      break;
    }
    ++iter;
  }
  if (iter != close) {
    throw ParserError::UnknownExpression;
  }
  ++iter;
  return ExprFnCall{.name = std::get<ExprSymbol>(sym).value,
                    .args = std::move(args)};
}

Expr Parser::parse_fndecl() {
  auto bkup = iter;
  auto kwfunc = std::get<ExprSymbol>(parse_symbol());
  if (kwfunc.value != "func") {
    iter = bkup;
    throw ParserError::FailTry;
  }
  trim();
  auto name = std::get<ExprSymbol>(parse_symbol());
  trim();
  if (!expect('('))
    throw ParserError::UnclosedParens;
  auto close = find_braces('(', ')');
  if (close == input.end()) {
    throw ParserError::UnclosedParens;
  }
  ++iter;
  std::vector<std::pair<std::string, std::string>> args;
  while (iter != close) {
    trim();
    auto arg = std::get<ExprSymbol>(parse_symbol());
    trim();
    auto type = std::get<ExprSymbol>(parse_symbol());
    args.push_back(std::make_pair(arg.value, type.value));
    trim();
    if (!expect(',')) {
      break;
    }
    ++iter;
  }
  if (iter != close) {
    throw ParserError::UnknownExpression;
  }
  ++iter;
  trim();
  if (!expect("->"))
    throw ParserError::ExpectedReturn;
  iter += 2;
  trim();
  auto ret = std::get<ExprSymbol>(parse_symbol());
  trim();
  if (!expect('{')) {
    throw ParserError::ExpectedBlock;
  }
  ++iter;
  auto body = parse_expr();
  if (!expect('}')) {
    throw ParserError::UnclosedCurlies;
  }
  ++iter;

  return ExprFnDecl{.name = name.value,
                    .args = std::move(args),
                    .ret_type = ret.value,
                    .body = std::make_shared<Expr>(body)};
}

MatchBranch Parser::parse_branch() {
  trim();
  auto pattern = parse_atom();
  trim();
  if (!expect("->"))
    throw ParserError::ExpectedReturn;
  iter += 2;
  trim();
  if (!expect('{'))
    throw ParserError::ExpectedBlock;
  ++iter;
  trim();
  auto expr = parse_expr();
  trim();
  if (!expect('}'))
    throw ParserError::ExpectedBlock;
  ++iter;
  return MatchBranch{.pattern = std::make_shared<Expr>(pattern),
                     .value = std::make_shared<Expr>(expr)};
}

Expr Parser::parse_match() {
  auto bkup = iter;
  auto kwmatch = std::get<ExprSymbol>(parse_symbol());
  if (kwmatch.value != "match") {
    iter = bkup;
    throw ParserError::FailTry;
  }
  trim();
  auto expr = parse_expr();
  trim();
  if (!expect('{'))
    throw ParserError::ExpectedBlock;
  auto close = find_braces('{', '}');
  if (close == input.end())
    throw ParserError::UnclosedCurlies;
  ++iter;
  std::vector<MatchBranch> branches{};
  while (iter != close) {
    trim();
    branches.push_back(parse_branch());
    trim();
    if (!expect(','))
      break;
    ++iter;
  }
  trim();
  if (iter != close) {
    throw ParserError::UnknownExpression;
  }
  ++iter;
  return ExprMatch{.value = std::make_shared<Expr>(expr),
                   .branches = std::move(branches)};
}