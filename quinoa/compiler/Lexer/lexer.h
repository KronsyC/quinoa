#pragma once
#include<vector>
#include "../token/token.h"

namespace Lexer{
    std::vector<Token> lexify(std::string source, std::string filename);
};