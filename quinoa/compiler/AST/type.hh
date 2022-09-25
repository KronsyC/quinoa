#pragma once
#include "./ast.hh"
#include "../../GenMacro.h"
#include<map>
#include "../token/TokenDef.h"
class Type: public AstNode{};

enum PrimitiveType{
    PRIMITIVES_ENUM_MEMBERS
};

static std::map<TokenType, PrimitiveType> primitive_mappings{
  PRIMITIVES_ENUM_MAPPINGS  
};

class Primitive:public Type{
public:
    PrimitiveType type;
    Primitive(PrimitiveType t){
        type = t;
    }
};

class CustomType:public Type{
public:
    Identifier* name;
    CustomType(Identifier* refersTo){
        name = refersTo;
    }
};
class TPtr:public Type{
public:
    Type* to;
    TPtr(Type* type){
        to = type;
    }
};