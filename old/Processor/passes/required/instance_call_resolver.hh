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





std::pair<bool, int> resolveInstanceCalls(CompilationUnit& unit){
    bool res = true;
    int count = 0;
    // inject 'this' param in signatures
    for(auto fn:unit.getAllMethods()){
        if(!fn->instance_access)continue;
        fn->instance_access = false;
        auto mod = fn->memberOf;
        auto modref = mod->get_ref();
        auto custom = new CustomType(new TLCType(modref));
        auto inst_ty = new ModuleInstanceType(custom);
        auto inst_ptr_ty = new TPtr(inst_ty);
        auto param = new Param(inst_ptr_ty, Ident::get("this"));
        (*fn->local_types)["this"] = inst_ptr_ty;
        count++;
        pushf(fn->sig->params, param);
    }
    // Rewrite instance calls as static calls with an additional argument
    for(auto fn:unit.getAllMethods()){
        for(auto src:fn->flatten()){
            if(auto binop = dynamic_cast<BinaryOperation*>(src)){
                if(auto call = dynamic_cast<MethodCall*>(binop->right)){
                    // Write the additional parameter
                    if(!call->inst)continue;
                    auto self = binop->left;
                    auto self_t = self->getType();
                    if(!self_t){
                        res = false;
                        continue;
                    }
                    auto selfptr = new UnaryOperation(self, PRE_amperand);
                    selfptr->ctx = call->ctx;

                    // Rewrite the name
                    auto inst = self_t->drill()->inst();
                    if(!inst)error("Methods may only be called on module instances");
                    auto mod = inst->of->drill()->mod();
                    if(!mod){
                        Logger::error("Unresolved Module");
                        res = false;
                        continue;
                    }
                    call->name->parent = mod->ref;
                    pushf(call->params, (Expression*)selfptr);
                    call->inst = false;
                    count++;
                }
            }
        }
    }
    return {res, count};
}