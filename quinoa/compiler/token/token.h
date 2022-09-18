#pragma once
#include<string>
#include "./TokenDef.h"
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
    Token() = default;

    bool is(TokenType type){
        return this->type == type;
    }

};