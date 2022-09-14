#pragma once
#include<string>
#include "../generated/TokenInfo.h"
class Token{
public:
    int col;
    int line;
    TokenType::TT type;
    std::string value;

    Token(TokenType::TT type, std::string value="", int line=0, int col=0){
        this->type = type;
        this->value = value;
        this->line=line;
        this->col=col;
    }

    bool is(TokenType::TT type){
        return this->type == type;
    }

};