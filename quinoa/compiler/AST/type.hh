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
private:
    Primitive(PrimitiveType t){
        type = t;
    }
public:
    PrimitiveType type;

    std::string str(){
        return primitive_names[type];
    }

    static Primitive* get(PrimitiveType t){
        static std::map<PrimitiveType, Primitive*> cache;
        auto fetched = cache[t];
        if(fetched == nullptr){
            auto prim = new Primitive(t);
            cache[t] = prim;
            return prim;
        }
        return fetched;
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
private:
    TPtr(Type* type){
        to = type;
    }
public:
    Type* to;

    std::string str(){
        return to->str()+"*";
    }

    static TPtr* get(Type* t){
        static std::map<Type*, TPtr*> cache;
        auto fetched = cache[t];
        if(fetched == nullptr){
            auto val = new TPtr(t);
            cache[t] = val;
            return val;
        }
        return fetched;
    }
};

class ListType:public Type{
private:
    ListType(Type* eT, Expression* n){
        elements = eT;
        size = n;
    }
public:
    Type* elements;
    Expression* size = nullptr;
    ListType() = default;
    std::string str(){
        return elements->str()+"[]";
    }
    static ListType* get(Type* t, Expression* n){
        static std::map<std::pair<Type*, Expression*>, ListType*> cache;
        auto fetched = cache[{t, n}];
        if(fetched == nullptr){
            auto val = new ListType(t, n);
            cache[{t, n}] = val;
            return val;
        }
        return fetched;
    }
};