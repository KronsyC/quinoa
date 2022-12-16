#include "../AST/primary.hh"
#include "../token/token.h"

class ConstantValue;

std::unique_ptr<ConstantValue> parse_const(Token tok);
std::unique_ptr<Expr> parse_expr(std::vector<Token> toks, Scope* parent);
