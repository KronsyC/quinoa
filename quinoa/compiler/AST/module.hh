#pragma once
#include "./ast.hh"
class Import:public TopLevelExpression{
public:
    CompoundIdentifier* target;
    Ident* member = nullptr;
    bool isStdLib = false;
    CompoundIdentifier* alias;

    Import(CompoundIdentifier* tgt, bool std, CompoundIdentifier* alias){
        target = tgt;
        isStdLib = std;
        this->alias = alias;
    }
    bool isImport(){
        return true;
    }
};



class Compositor:public ModuleRef{
public:
    Block<Expression> params;
};

class Method;
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    Block<Generic> generics;
    // The name of the Module, i.e 'Test', 'MyModule'
    Ident* name;

    // The Namespace Unique hash for the module
    // i.e abcdefg
    Ident* nspace;
    CompoundIdentifier* fullname(){
        // if(!nspace)error("The module " + name->str() + " does not have a namespace");
        auto id = new CompoundIdentifier();
        if(nspace)id->push_back(nspace);
        id->push_back(name);
        return id;
    }
    Block<Compositor> compositors; 
    bool isModule(){
        return true;
    }

    Compositor* comp(std::string comp){
        for(auto c:compositors){
            if(c->name->str() == comp)return c;
        }
        return nullptr;
    } 
    void remove(std::string comp){
        int idx = -1;
        for(int i  = 0;i<compositors.size();i++){
            auto c = compositors[i];
            if(c->name->str() == comp){
                idx = i;
                break;
            }
        }
        if(idx!=-1){
            
            compositors.erase(compositors.begin()+idx);
        }
    }

    std::vector<Method*> getAllMethods(){
        std::vector<Method*> ret;
        for(auto m:*this){
            auto mt = (Method*)m;
            if(instanceof<Method>(m))ret.push_back(mt);
        }
        return ret;
    }
};



