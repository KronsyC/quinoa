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
    std::string file;

    Token(TokenType type, std::string value="", int line=0, int col=0, std::string filename=""){
        this->type = type;
        this->value = value;
        this->line=line;
        this->file = filename;
        this->col=col;
    }
    Token() = default;

    bool is(TokenType type){
        return this->type == type;
    }
    bool isTypeTok(){
        for(auto t:defs){
            if(t->ttype == type)return t->type != "";
        }
        return false;
    }
    bool isIndentationTok(){
        for(auto d:defs){
            if(d->ttype == type)return d->ind;
        }
        return false;
    }
    bool isDeIndentationTok(){
        for(auto d:defs){
            if(d->ttype == type)return d->dind;
        }
        return false;
    }
    std::string position(){
        return "("+file+":"+std::to_string(line)+":"+std::to_string(col)+")";
    }

    std::string afterpos(){
        return "["+std::to_string(line) + ":" + std::to_string(col+value.size()) + "]";

    }

};

