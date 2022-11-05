#pragma once
#include "../include.h"
/**
 * 
 * This pass is responsible for generating generic implementations of functions
 * 
 */

std::vector<MethodCall*> get_all_calls(CompilationUnit& unit){
    std::vector<MethodCall*> calls;
    for(auto fn: unit.getAllMethods()){
        auto flat = fn->flatten();
        for(auto stm:flat){
            if(instanceof<MethodCall>(stm)){
                auto call = (MethodCall*)stm;
                if(!includes(calls, call))calls.push_back(call);
            }
        }
    }
    return calls;
}

void impl_generics(CompilationUnit& unit){
    auto calls = get_all_calls(unit);
    for(auto call:calls){
        if(!call->target)continue;
        if(!call->target->isGeneric())continue;

        call->target->belongsTo->genFor(call->target);
    }

}