#pragma once
#include "../processor.h"
//
// Validates all literal array instances to ensure that
// 1: All arrays have the proper type
// 2: Arrays are only used in initializers to variables
// 3: ?
//


void validateLiteralArrays(CompilationUnit& unit){
    for(auto method:unit.getAllMethods()){
        auto flat = method->flatten();

        for(auto stm:flat){
            if(instanceof<InitializeVar>(stm)){
                auto init = (InitializeVar*)stm;
                if(!init->initializer)continue;
                if(auto type=init->type->list()){
                    // if the right is a list literal, cast it
                    if(!instanceof<List>(init->initializer))continue;
                    auto list = (List*)init->initializer;
                    list->setElementsType(type->elements);
                }
            }            
        }
    }
}