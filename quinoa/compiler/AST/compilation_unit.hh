#pragma once
#include "./include.hh"
#include "./container.hh"


class Module;

class CompilationUnit: public ANode{
public:
    Vec<TopLevelEntity> members;

    std::vector<Module*> get_modules(){
        std::vector<Module*> ret;
        for(auto& m:members){
            if(auto mod = dynamic_cast<Module*>(&m)){
                ret.push_back(mod);
            }            
        }
        return ret;
    }
};