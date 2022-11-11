/**
 * Ensure that variables are only used after they are declared
*/

#pragma once
#include "../include.h"


void validate_usage(Scope& scope, std::vector<std::string> declared_vars){
    for(auto member : scope.flatten()){
        if(member->scope == &scope){
            if(auto nest = dynamic_cast<Scope*>(member)){
                validate_usage(*nest, declared_vars);
                continue;
            }
            if(auto init = dynamic_cast<InitializeVar*>(member)){
                declared_vars.push_back(init->var_name.str());
            }

            if(auto var = dynamic_cast<SourceVariable*>(member)){
                if(!includes(declared_vars, var->name->str())){
                    except(E_BAD_VAR, "Cannot use variable '"+var->name->str()+"' before initialization");
                }
            }

        }
    }
}

void validate_variable_usage(CompilationUnit& unit){
    for(auto method : unit.get_methods()){
        if(!method->content)continue;
        std::vector<std::string> declarations;
        for(auto& p : method->parameters){
            declarations.push_back(p->name.str());
        }
        for(auto prop : unit.get_properties()){
            declarations.push_back(prop->name->str());

            if(prop->name->container == method->name->container){
                declarations.push_back(prop->name->member->str());
            }
        }

        validate_usage(*method->content, declarations);
    }
}