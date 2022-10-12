#pragma once
#include "./ast.hh"

struct CompilationUnit : public Block<TopLevelExpression>
{
public:
    CompilationUnit(){
        this->take();
    }
    Block<Module> getAllModules(){
        Block<Module> ret(false);
        for(auto child:*this){
            if(child==nullptr)continue;
            auto isMod = child->isModule();

            if(isMod)ret.push_back((Module*)child);
           }
        return ret;
    }

    Block<Method> getAllMethods(){
        Block<Method> ret(false);
        auto mods = getAllModules();
        for(auto mod:mods){
            for(auto child:*mod){
                if(instanceof<Method>(child))ret.push_back((Method*)child);
            }
        }
        return ret;
    }
};