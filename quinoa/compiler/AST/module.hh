#pragma once
#include "./ast.hh"
class Import:public TopLevelExpression{
public:
    CompoundIdentifier* target;
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
class ModuleReference:public Block<Expression>{
public:
    Identifier* name;
};
class Method;
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    CompoundIdentifier* name;
    std::vector<ModuleReference*> compositors; 
    bool undergone_appropriate_processing = false;
    bool isModule(){
        return true;
    }

    bool is(std::string comp){
        for(auto c:compositors){
            if(c->name->str() == comp)return true;
        }
        return false;
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



