#pragma once
#include<vector>
#include "./Token.h"

namespace Lexer{
    std::vector<Token> lexify(std::string source);
};