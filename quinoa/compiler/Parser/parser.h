#pragma once
#include<vector>
#include "../token/token.h"
#include "../AST/ast.hh"
namespace Parser{
    CompilationUnit makeAst(std::vector<Token>& tokens);
}