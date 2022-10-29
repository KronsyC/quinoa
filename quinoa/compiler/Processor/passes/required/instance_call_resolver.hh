#pragma once
#include "../include.h"
/**
 * This transformation step is vital, transforms an expression such as:
 * 
 * obj.dosomething(...args);
 * into:
 * objbaseclass::dosomething(&obj, ...args);
 * 
 * Also Injects `this` parameter into instance methods
*/





void resolveInstanceCalls(CompilationUnit& unit){

    // inject 'this' param in signatures
    for(auto fn:unit.getAllMethods()){
        if(!fn->instance_access)continue;
        fn->instance_access = false;
        auto mod = fn->memberOf;
        auto modref = mod->get_ref();
        auto custom = new CustomType(new ModuleType(modref));
        auto inst_ty = new ModuleInstanceType(custom);
        auto inst_ptr_ty = new TPtr(inst_ty);
        auto param = new Param(inst_ptr_ty, Ident::get("this"));
        pushf(fn->sig->params, param);
    }

    // Rewrite instance calls as static calls with an additional argument
    for(auto fn:unit.getAllMethods()){
        for(auto src:fn->flatten()){
            if(auto binop = dynamic_cast<BinaryOperation*>(src)){
                if(auto call = dynamic_cast<MethodCall*>(binop->right)){
                    Logger::debug("Found a call");
                    binop->active = false;
                    call->active = false;
                }
            }
        }
    }
}