#pragma once
#include "./ast.hh"

class Import:public TopLevelExpression{
public:
    Identifier* target;
    bool isStdLib = false;
    Identifier* alias;

    Import(Identifier* tgt, bool std, Identifier*alias){
        target = tgt;
        isStdLib = std;
        this->alias = alias;
    }
};
class ModuleReference:public Block<Expression>{
    Identifier* name;

};
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    Identifier* name;
    std::vector<ModuleReference*> compositors;    
};

class Param : public AstNode{
public:
    Type* type;
    Ident* name;
    Param(Type* type, Ident* name){
        this->type = type;
        this->name = name;
    }
};

class Method:public ModuleMember, public Block<Statement>{
public:
    Identifier* name;
    std::vector<Param*> params;
    Type* returnType;
};

// Method definitions are methods without implementations
// used primarily for hoisting
class MethodDefinition:public TopLevelExpression{
public:
    Identifier* name;
    std::vector<Param*> params;
    Type* returnType;
};