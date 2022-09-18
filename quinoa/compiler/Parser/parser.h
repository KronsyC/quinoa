#pragma once
#include<vector>
#include "../token/token.h"

namespace Parser{
    void* makeAst(std::vector<Token>& tokens);
}