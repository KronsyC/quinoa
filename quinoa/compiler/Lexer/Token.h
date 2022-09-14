#pragma once
#include<string>
#include "../generated/TokenDef.h"
class Token{
public:
    int col;
    int line;
    TokenType type;
    std::string value;

    Token(TokenType type, std::string value="", int line=0, int col=0){
        this->type = type;
        this->value = value;
        this->line=line;
        this->col=col;
    }

    bool is(TokenType type){
        return this->type == type;
    }

};