#pragma once
#include "../token/token.h"
#include <vector>

namespace Lexer
{
std::vector<Token> lexify(std::string source, std::string filename);
};