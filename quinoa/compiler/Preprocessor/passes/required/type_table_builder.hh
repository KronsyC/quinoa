#pragma once

#include "../include.h"



void build_type_table(CompilationUnit& unit){
    for(auto method : unit.get_methods()){
        if(!method->content)continue;
        // Inject parameter types
        for(auto param : method->parameters){
            method->content->set_type(param->name.str(), param->type);
        }

        for(auto node : method->content->flatten()){
            if(auto init = dynamic_cast<InitializeVar*>(node)){
                if(!init->type)except(E_INTERNAL, "Initializer of " + init->var_name.str() + " has no type");

                init->scope->set_type(init->var_name.str(), init->type);
            }


        }

    }
}