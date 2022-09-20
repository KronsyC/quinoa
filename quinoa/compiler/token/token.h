#pragma once
#include<string>
#include<vector>
#include "./TokenDef.h"
#include "../../lib/error.h"
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
    bool isTypeTok(){
        for(auto t:defs){
            if(t->ttype == type)return t->type;
        }
        error("Failed To Locate Token Entry");
    }
    std::string pos(){
        return "["+std::to_string(line) + ":" + std::to_string(col) + "]";
    }

    std::string afterpos(){
        return "["+std::to_string(line) + ":" + std::to_string(col+value.size()) + "]";

    }

};

