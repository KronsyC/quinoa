#pragma once
#include "../token/token.h"
#include <vector>
namespace Parser
{
CompilationUnit* makeAst(std::vector<Token>& tokens);
}