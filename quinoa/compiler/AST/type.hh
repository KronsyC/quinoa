#pragma once
#include "./ast.hh"
#include "../../GenMacro.h"
#include<map>
#include "../token/TokenDef.h"

enum PrimitiveType{
    PRIMITIVES_ENUM_MEMBERS
};

static std::map<TokenType, PrimitiveType> primitive_mappings{
  PRIMITIVES_ENUM_MAPPINGS  
};
static std::map<PrimitiveType, std::string> primitive_names{
    PRIMITIVES_ENUM_NAMES
};
class Primitive:public Type{
public:
    PrimitiveType type;
    Primitive(PrimitiveType t){
        type = t;
    }
    std::string str(){
        return primitive_names[type];
    }
    bool equals(Type* type){
        if(instanceof<Primitive>(type)){
            auto typ = (Primitive*)type;
            return typ->type == this->type;
        }
        return false;
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
    std::string str(){
        return to->str()+"*";
    }
    bool equals(Type* type){
        if(instanceof<TPtr>(type)){
            auto typ = (TPtr*)type;
            return to->equals(typ->to);
        }
        return false;
    }
};

class ListType:public Type{
public:
    Type* elements;
    Expression* size = nullptr;
    ListType(Type* eT, Expression* n){
        elements = eT;
        size = n;
    }
    std::string str(){
        return elements->str()+"[]";
    }
    bool equals(Type* type){
        if(instanceof<ListType>(type)){
            auto typ = (ListType*)type;
            return elements->equals(typ->elements);
        }
        return false;
    }
};