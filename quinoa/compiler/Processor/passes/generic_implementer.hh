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

        // Generate the implementation of the generic by:
        // a: Locating the method that needs to be re-implemented
        // b: Generating the new method
        // c: changing the target to the new method

        for(auto m :methods){
            if(m->sig != call->target)continue;
            Logger::debug("Found the base function");
            break;
        }
        Logger::debug("Call to generic " + call->name->str());
    }

    // disable all generic methods
    for(auto m:methods){
        if(!m->sig->isGeneric())continue;
        m->generate = false;
        Logger::debug("Generic Function " + m->sig->name->str());
    }
}