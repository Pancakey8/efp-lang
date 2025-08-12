#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

struct ExprNumber {
  float value;
};

struct ExprString {
  std::string value;
};

struct ExprSymbol {
  std::string value;
};

struct ExprOperation;
struct ExprFnCall;
struct ExprFnDecl;
struct ExprMatch;

enum class Operator { Add, Sub, Mul, Div };

using Expr = std::variant<ExprNumber, ExprString, ExprSymbol, ExprFnCall,
                          ExprOperation, ExprFnDecl, ExprMatch>;

struct ExprOperation {
  std::shared_ptr<Expr> left;
  std::shared_ptr<Expr> right;
  Operator op;
};

struct ExprFnCall {
  std::string name;
  std::vector<Expr> args;
};

struct ExprFnDecl {
  std::string name;
  std::vector<std::pair<std::string, std::string>> args; // name : type
  std::string ret_type;
  std::shared_ptr<Expr> body;
};

struct MatchBranch {
  std::shared_ptr<Expr> pattern;
  std::shared_ptr<Expr> value;
};

struct ExprMatch {
  std::shared_ptr<Expr> value;
  std::vector<MatchBranch> branches;
};

enum class ParserError {
  FailTry,
  UnclosedQuote,
  UnknownExpression,
  UnclosedParens,
  ExpectedBlock,
  UnclosedCurlies,
  ExpectedReturn
};

class Parser {
  std::string input;
  std::string::iterator iter;

  bool expect(const std::function<bool(char)> &&fn);
  bool expect(const char &&c);
  bool expect(const std::string &&str);
  void trim();

  std::string::iterator find_braces(const char &&left, const char &&right);

  Expr parse_number();
  Expr parse_string();
  Expr parse_op0();
  Expr parse_op1();
  Expr parse_operation();
  Expr parse_symbol();
  Expr parse_fncall();
  Expr parse_fndecl();
  MatchBranch parse_branch();
  Expr parse_match();
  Expr parse_atom();
  Expr parse_expr();

public:
  std::vector<Expr> output{};

  Parser(const std::string &&input)
      : input(std::move(input)), iter(this->input.begin()) {}

  void parse_all();
};

void print_expr(const Expr &e);