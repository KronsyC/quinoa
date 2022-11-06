#pragma once
#include "./include.hh"
#include "./container.hh"


class CompilationUnit: public ANode{
public:
    Vec<TopLevelEntity> members;

    std::vector<Container*> get_containers(){
        std::vector<Container*> ret;
        for(auto m:members){
            if(auto mod = dynamic_cast<Container*>(m)){
                ret.push_back(mod);
            }            
        }
        return ret;
    }

    std::vector<Method*> get_methods(){
        std::vector<Method*> ret;
        for(auto* cont : get_containers()){
            for(auto method : cont->get_methods()){
                ret.push_back(method);
            }
        }
        return ret;
    }
};