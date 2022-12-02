#pragma once
#include "../include.h"


void normalize_types(CompilationUnit& unit){
    for(auto method : unit.get_methods()){
        if(!method->content)continue;
        for(auto code : method->content->flatten()){
            if(auto expr = dynamic_cast<Expr*>(code)){
                expr->normalize();
            }
        }
    }
}
