#pragma once
#include "./ast.hh"
#include "../../GenMacro.h"

class Type: public AstNode{};

enum PrimitiveType{
    PRIMITIVES_ENUM_MEMBERS
};

class Primitive:public Type{
public:
    PrimitiveType type;
    Primitive(PrimitiveType t){
        type = t;
    }
};
class TPtr:public Type{
    Type* to;
};