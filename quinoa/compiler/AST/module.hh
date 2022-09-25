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
public:
    Identifier* name;

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
    // Used to locally identify the method
    Ident* name;
    Identifier* fullname;
    std::vector<Param*> params;
    Type* returnType;
};
class Entrypoint:public TopLevelExpression, public Method{
public:
    Identifier* calls;
    Entrypoint(Identifier* calls){
        this->calls = calls;
    }

};
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    Identifier* name;
    std::vector<ModuleReference*> compositors; 


    bool is(std::string comp){
        for(auto c:compositors){
            if(c->name->str() == comp)return true;
        }
        return false;
    }  
    Method* getMethod(std::string name){
        for(auto m:this->items){
            auto mt = (Method*)m;
            if(instanceof<Method>(m) && mt->name->str() == name)return mt;

        }
        return nullptr;
    }
    bool hasMethod(std::string name){
        return this->getMethod(name) != nullptr;
    } 
};



// Method definitions are methods without implementations
// used primarily for hoisting
class MethodDefinition:public TopLevelExpression{
public:
    Identifier* name;
    std::vector<Param*> params;
    Type* returnType;
};