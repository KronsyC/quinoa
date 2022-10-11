#pragma once

//
//	This Process is responsible for injecting all of the primitive functions into the ast
// 	(denoted as dunder methods)
// 	Primitive Functions include __syscall__ and __cast__<T>
//  they are all defined in the lang.primitives module
//

#include "../processor.h"

void implIntrinsics(CompilationUnit &unit)
{

    // glibc syscall
    {
        auto sig = new MethodSignature;
        sig->nomangle = true;
        std::vector<Param *> params;
        auto i64 = Primitive::get(PR_int64);
        params.push_back(new Param(i64, Ident::get("callno")));
        auto varargs = new Param(ListType::get(i64), Ident::get("params"));
        varargs->isVariadic = true;
        params.push_back(varargs);
        sig->name = Ident::get("syscall");
        sig->returnType = i64;
        sig->params = params;

        pushf(unit.items, (TopLevelExpression *)new MethodPredeclaration(sig));
    }
}
static auto n1 = new Integer(1);
static auto n2 = new Integer(2);
static auto n3 = new Integer(3);
static auto n4 = new Integer(4);
static auto n5 = new Integer(5);
static auto n6 = new Integer(6);
MethodCall *getCall(MethodCall templ, Identifier *args, int n)
{

    auto call = new MethodCall(templ);
    if (n >= 1)
        call->params.push_back(new Subscript(args, n1));
    if (n >= 2)
        call->params.push_back(new Subscript(args, n2));
    if (n >= 3)
        call->params.push_back(new Subscript(args, n3));
    if (n >= 4)
        call->params.push_back(new Subscript(args, n4));
    if (n >= 5)
        call->params.push_back(new Subscript(args, n5));
    if (n >= 6)
        call->params.push_back(new Subscript(args, n6));

    return call;
}

void implSyscall(Method *method, CompilationUnit &unit)
{

    MethodCall call;
    auto varargs = method->sig->params[1]->name;
    auto argCount = new ArrayLength(varargs);

    call.ctx = method;
    call.name = new CompoundIdentifier("syscall");
    call.nomangle = true;
    call.params.push_back(method->sig->params[0]->name);
    auto call1 = getCall(call, varargs, 1);
    auto call2 = getCall(call, varargs, 2);
    auto call3 = getCall(call, varargs, 3);
    auto call4 = getCall(call, varargs, 4);
    auto call5 = getCall(call, varargs, 5);
    auto call6 = getCall(call, varargs, 6);

    // Generate the code blocks
    auto cb1 = new SourceBlock(new Return(call1));
    auto cb2 = new SourceBlock(new Return(call2));
    auto cb3 = new SourceBlock(new Return(call3));
    auto cb4 = new SourceBlock(new Return(call4));
    auto cb5 = new SourceBlock(new Return(call5));
    auto cb6 = new SourceBlock(new Return(call6));
    auto eq = BIN_equals;
    // generate the conditional
    auto conditional = new IfCond(
        new BinaryOperation(argCount, n1, eq),
        cb1,
        new SourceBlock(new IfCond(
            new BinaryOperation(argCount, n2, eq),
            cb2,
            new SourceBlock(new IfCond(
                new BinaryOperation(argCount, n3, eq),
                cb3,
                new SourceBlock(new IfCond(
                    new BinaryOperation(argCount, n4, eq),
                    cb4,
                    new SourceBlock(new IfCond(
                        new BinaryOperation(argCount, n5, eq),
                        cb5,
                        new SourceBlock(new IfCond(
                            new BinaryOperation(argCount, n6, eq),
                            cb6,
                            cb6)))))))))));
    // method->items.push_back(conditional);
}

void implementPrimitive(Method *method, CompilationUnit &unit)
{
    auto name = method->sig->name->str();
    if (name == "__syscall__")
        implSyscall(method, unit);
};

void injectPrimitiveFunctions(CompilationUnit &unit)
{
    implIntrinsics(unit);
    for (auto mod : unit.getAllModules())
    {
        auto modname = mod->name->last();
        if (mod->is("CompilerImplemented") && modname->str() == "primitives")
        {
            for (auto method : mod->getAllMethods())
            {
                implementPrimitive(method, unit);
            }
        }
    }
}