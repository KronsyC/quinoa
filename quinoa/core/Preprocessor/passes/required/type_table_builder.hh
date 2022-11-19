#pragma once

#include "../include.h"

void build_method_type_table(Method* method, CompilationUnit& unit){
    if(!method->content)return;



    // Inject  properties
    for(auto prop:unit.get_properties()){
        method->content->set_type(prop->name->str(), prop->type);

        if(prop->parent == method->parent){
            method->content->set_type(prop->name->member->str(), prop->type);
        }
    }

    // Inject local enum definitions
    for(auto type : unit.get_types()){
        if(auto _enum = dynamic_cast<EnumType*>(type->refers_to.get())){
            for(auto member : _enum->entries){

                auto full_name = type->name->str() + "::" + member;
                method->content->set_type(full_name, type->refers_to);

                if(type->parent == method->parent){
                    method->content->set_type(type->name->member->str() + "::" + member, type->refers_to);
                }

            }

        }
    }

    // Inject parameter types
    for(auto param : method->parameters){
        method->content->set_type(param->name.str(), param->type);
    }
    // Inject self (if applicable)
    if(method->acts_upon){
        method->content->set_type("self", Ptr::get(method->acts_upon));
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