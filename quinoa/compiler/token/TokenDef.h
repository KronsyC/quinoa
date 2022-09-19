
#pragma once

#include<string>
#include<vector>
#include "../../GenMacro.h"

enum TokenType{
	DEFINITIONS_ENUM_MEMBERS
};


class TokenDefinition{
public:
    TokenType ttype;
    std::string name;
    
	DEFINITIONS_STR;

    TokenDefinition(TokenType ttype, std::string name DEFINITIONS_ARGS){
        this->ttype = ttype;
		this->name=name;
		DEFINITIONS_DEFAULT_ASSIGNMENTS;
    }
};


static std::vector<TokenDefinition*> defs{
    DEFINITIONS_INITIALIZERS
};
