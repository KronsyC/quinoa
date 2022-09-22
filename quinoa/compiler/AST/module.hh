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
    Type* type;
    Ident* name;
};

class Method:public ModuleMember, Block<Statement>{
public:
    Ident* name;
    std::vector<Param*> params;
};