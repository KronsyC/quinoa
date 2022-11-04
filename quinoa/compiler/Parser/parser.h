#pragma once
#include "../AST_old/ast.hh"
#include "../token/token.h"
#include <vector>
namespace Parser
{
CompilationUnit* makeAst(std::vector<Token>& tokens);
}