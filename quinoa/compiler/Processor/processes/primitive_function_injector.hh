#pragma once

//
//	This Process is responsible for injecting all of the primitive functions into the ast
// 	(denoted as dunder methods)
// 	Primitive Functions include __syscall__ and __cast__<T>
//  they are all defined in the lang.primitives module
//

#include "../processor.h"

void implSyscall(Method *method, CompilationUnit& unit){
    auto sig = new MethodSignature;
    sig->nomangle = true;
    std::vector<Param*> params;
    auto i64 = Primitive::get(PR_int64);
    params.push_back(new Param(i64, Ident::get("callno")));
    auto varargs = new Param(i64, Ident::get("params"));
    varargs->isVariadic = true;
    params.push_back(varargs);
    sig->name = Ident::get("syscall");
    sig->returnType = i64;
    sig->params = params;

    pushf(unit.items, (TopLevelExpression*)new MethodPredeclaration(sig));


    Logger::debug("Implementing Syscall Fn");
    auto call = new MethodCall;
    call->ctx = method;
    call->name = new CompoundIdentifier("syscall");
    call->nomangle = true;
    method->items.push_back(call);
    method->items.push_back(new Return(new Integer(11)));
    
}


void implementPrimitive(Method *method, CompilationUnit& unit){
    auto name = method->sig->name->str();
    if(name=="__syscall__")implSyscall(method, unit);
};

void injectPrimitiveFunctions(CompilationUnit &unit)
{
    for (auto mod : unit.getAllModules())
    {
        auto modname = mod->name->last();
        if (mod->is("CompilerImplemented") && modname->str() == "primitives")
        {
            Logger::debug("Found Primitives mod");
            for (auto method : mod->getAllMethods())
            {
                implementPrimitive(method, unit);
            }
        }
    }
}