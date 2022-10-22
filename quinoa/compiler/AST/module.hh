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
class Module;

class ModuleReference:public Block<Expression>{
public:
    Identifier* name;
    Module* refersTo;
    Block<Expression> params;
};
class Method;
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    Block<Generic> generics;
    CompoundIdentifier* name;
    std::vector<ModuleReference*> compositors; 
    bool undergone_appropriate_processing = false;
    bool isModule(){
        return true;
    }

    ModuleReference* comp(std::string comp){
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



