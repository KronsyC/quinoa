#pragma once
#include "../processor.h"
#include "../../../lib/list.h"
/**
 * 
 * This pass is responsible for generating generic implementations of functions
 * 
 */

std::vector<MethodCall*> getAllCalls(CompilationUnit& unit){
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

void implGenerics(CompilationUnit& unit){
    auto calls = getAllCalls(unit);
    auto methods = unit.getAllMethods();
    for(auto call:calls){
        if(!call->target)continue;
        if(!call->target->isGeneric())continue;

        Logger::debug("Call to generic " + call->name->str());
        call->target->belongsTo->genFor(call->target);
    }

}