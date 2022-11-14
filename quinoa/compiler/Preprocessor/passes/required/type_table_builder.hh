#pragma once

#include "../include.h"


void build_method_type_table(Method* method, CompilationUnit& unit){
    if(!method->content)return;
    // Inject local properties
    for(auto prop:unit.get_properties()){
        method->content->set_type(prop->name->str(), prop->type);

        if(prop->name->container == method->name->container){
            method->content->set_type(prop->name->member->str(), prop->type);
        }
    }
    // Inject parameter types
    for(auto param : method->parameters){
        method->content->set_type(param->name.str(), param->type);
    }

    // Inject local variables
    for(auto node : method->content->flatten()){
        if(auto init = dynamic_cast<InitializeVar*>(node)){
            if(!init->type)return;

            init->scope->set_type(init->var_name.str(), init->type);
        }


    }
}
void build_type_table(CompilationUnit& unit){
    for(auto method : unit.get_methods()){
        build_method_type_table(method, unit);
    }
}