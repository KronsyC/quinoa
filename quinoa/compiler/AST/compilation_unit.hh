#pragma once
#include "./ast.hh"

struct CompilationUnit : public Block<TopLevelExpression>
{
public:
    std::vector<Module*> getAllModules(){
        std::vector<Module*> ret;
        for(auto child:items){
            if(instanceof<Module>(child)){
                ret.push_back((Module*)child);
            }        
           }
        return ret;
    }

    std::vector<Method*> getAllMethods(){
        std::vector<Method*> ret;
        auto mods = getAllModules();
        for(auto mod:mods){
            for(auto child:mod->items){
                if(instanceof<Method>(child))ret.push_back((Method*)child);
            }
        }
        return ret;
    }
};