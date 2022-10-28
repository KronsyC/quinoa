#include "./codegen.hh"
#include "../AST/ast.hh"
#include "../../lib/error.h"
#include <string>
#include <algorithm>
#include <map>
#include "llvm/Linker/Linker.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "../../lib/logger.h"
#include "../../lib/list.h"
using namespace std;

llvm::Function *make_fn(MethodSignature &f, llvm::Module *mod, llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage, bool mangle = true)
{
    auto ret = f.returnType->getLLType();
    auto name = mangle ? f.sourcename() : f.name->str();
    vector<llvm::Type *> args;
    for (auto a : f.params)
        args.push_back(a->type->getLLType());
    bool isVarArg = false;
    if (f.params.size())
    {
        auto last = f.params[f.params.size() - 1];
        if (last->isVariadic)
        {
            isVarArg = true;
            args.pop_back();
            if (!f.nomangle)
            {
                auto t = Primitive::get(PR_int32);
                args.push_back(t->getLLType());
                auto arglen = Ident::get("+vararg_count");
                f.params[f.params.size() - 1] = new Param(t, arglen);
                //
            }
        }
    }

    auto sig = llvm::FunctionType::get(ret, args, isVarArg);
    auto fn = llvm::Function::Create(sig, linkage, name, mod);
    // Logger::debug("Created function " + name);
    for (unsigned int i = 0; i < fn->arg_size(); i++)
        fn->getArg(i)->setName(f.params[i]->name->str());
    return fn;
}

llvm::GlobalValue *make_global(Property *prop, llvm::Module *mod, llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage)
{
    Logger::debug("Create global " + prop->str());
    auto type = prop->type->getLLType();
    auto initializer = prop->initializer;
    llvm::Constant* const_initializer = nullptr;
    if(initializer){
        auto constant = initializer->constant();
        if(!constant)error("Initialzers must be constant");
        const_initializer = constant->getLLConstValue(type);
    }
    auto global = new llvm::GlobalVariable(*mod, type, false, linkage, const_initializer, prop->str());
    return global;
}
struct ControlFlowInfo
{
    // The block to jump to after the `break` action is invoked
    llvm::BasicBlock *breakTo;

    // The block to jump to after the `continue` action is invoked
    llvm::BasicBlock *continueTo;

    // The block to jump to after the `fallthrough` action is invoked
    llvm::BasicBlock *fallthroughTo;

    // The block to break to after the inner scope is executed
    llvm::BasicBlock *exitBlock;
};

void gen_src(vector<Statement *> content, llvm::Function *func, TVars vars, ControlFlowInfo cfi = {})
{
    Logger::debug("Generating Source of " + func->getName().str());
    for (auto stm : content)
    {
        if (!stm->active)
            continue;
        if (instanceof <InitializeVar>(stm))
        {
            auto init = (InitializeVar *)stm;
            auto varname = init->varname->str();
            auto type = init->type->getLLType();
            llvm::AllocaInst *alloca;
            if (init->type->list())
            {
                auto l = init->type->list();
                llvm::Value *size = l->size->getLLValue(vars, Primitive::get(PR_int32)->getLLType());
                if (!size)
                    error("You must know the size of an array while allocating it");

                // decide on static or dynamic type
                if (l->isStatic())
                {
                    if (! instanceof <Integer>(l->size))
                        error("Cannot initialize list with non-integer length");
                    auto size = ((Integer *)l->size)->value;
                    auto arrayType = llvm::ArrayType::get(l->elements->getLLType(), size);
                    alloca = builder()->CreateAlloca(arrayType);
                }
                else
                {
                    alloca = builder()->CreateAlloca(type->getPointerElementType(), size, "list " + varname);
                }
            }
            else
                alloca = builder()->CreateAlloca(type, nullptr, "var " + varname);

            vars[varname] = alloca;
        }
        else if (instanceof <WhileCond>(stm))
        {
            auto loop = (WhileCond *)stm;

            auto evaluatorBlock = llvm::BasicBlock::Create(*llctx(), "while_eval", func);
            auto runBlock = llvm::BasicBlock::Create(*llctx(), "while_exec", func);
            auto continuationBlock = llvm::BasicBlock::Create(*llctx(), "while_cont", func);

            ControlFlowInfo cf = cfi;
            cf.breakTo = continuationBlock;
            cf.continueTo = evaluatorBlock;
            cf.exitBlock = continuationBlock;

            // Set up the evaluator
            builder()->CreateBr(evaluatorBlock);
            builder()->SetInsertPoint(evaluatorBlock);
            auto cond = loop->cond->getLLValue(vars, Primitive::get(PR_boolean)->getLLType());
            builder()->CreateCondBr(cond, runBlock, continuationBlock);

            // Set up the content
            builder()->SetInsertPoint(runBlock);
            gen_src(*loop, func, vars, cf);
            builder()->CreateBr(evaluatorBlock);

            // generate the continuation
            builder()->SetInsertPoint(continuationBlock);
        }
        else if (instanceof <IfCond>(stm))
        {
            auto iff = (IfCond *)stm;

            auto execBlock = llvm::BasicBlock::Create(*llctx(), "if_exec", func);
            auto continuationBlock = llvm::BasicBlock::Create(*llctx(), "if_cont", func);
            auto elseBlock = iff->otherwise ? llvm::BasicBlock::Create(*llctx(), "if_else", func) : continuationBlock;
            ControlFlowInfo cf = cfi;
            cf.exitBlock = continuationBlock;

            auto cond = iff->cond->getLLValue(vars, Primitive::get(PR_boolean)->getLLType());
            builder()->CreateCondBr(cond, execBlock, elseBlock);

            builder()->SetInsertPoint(execBlock);
            gen_src(*iff->does, func, vars, cf);
            builder()->CreateBr(continuationBlock);

            if (iff->otherwise)
            {
                builder()->SetInsertPoint(elseBlock);
                gen_src(*iff->otherwise, func, vars, cf);
                builder()->CreateBr(continuationBlock);
            }
            builder()->SetInsertPoint(continuationBlock);
        }
        else if (instanceof <ForRange>(stm))
        {
            auto loop = (ForRange *)stm;

            auto evaluatorBlock = llvm::BasicBlock::Create(*llctx(), "for_eval", func);
            auto incrementBlock = llvm::BasicBlock::Create(*llctx(), "for_inc", func);
            auto runBlock = llvm::BasicBlock::Create(*llctx(), "for_exec", func);
            auto continuationBlock = llvm::BasicBlock::Create(*llctx(), "while_cont", func);

            ControlFlowInfo cf = cfi;
            cf.breakTo = continuationBlock;
            cf.continueTo = incrementBlock;
            cf.exitBlock = continuationBlock;

            // Set up the evaluator
            builder()->CreateBr(evaluatorBlock);
            builder()->SetInsertPoint(evaluatorBlock);
            auto cond = loop->cond->getLLValue(vars, Primitive::get(PR_boolean)->getLLType());
            builder()->CreateCondBr(cond, runBlock, continuationBlock);

            // Set up the content
            builder()->SetInsertPoint(runBlock);
            gen_src(*loop, func, vars, cf);
            builder()->CreateBr(incrementBlock);

            builder()->SetInsertPoint(incrementBlock);
            gen_src(*loop->inc, func, vars);
            builder()->CreateBr(evaluatorBlock);
            // generate the continuation
            builder()->SetInsertPoint(continuationBlock);
        }
        else if (instanceof <Return>(stm))
        {
            auto ret = (Return *)stm;
            if(ret->retValue){
            auto expr = ret->retValue->getLLValue(vars, func->getReturnType());
            builder()->CreateRet(cast(expr, func->getReturnType()));
            }
            else builder()->CreateRetVoid();
            
        }
        // interpret as expression with dumped value
        else if (instanceof <Expression>(stm))
        {
            ((Expression *)stm)->getLLValue(vars);
        }
        else
            error("Failed Generate IR for statement");
    }
}
TVars inject_vars(llvm::Function *fn, CompilationUnit& ast, Method *method)
{
    if (fn == nullptr)
        error("Cannot varify the args of a null function", true);
    TVars vars;

    // Inject the peer properties as non-prefixed variables
    for(auto prop:method->memberOf->getAllProperties()){
        auto prop_name = prop->name->member;
        vars[prop_name->str()] = (llvm::AllocaInst*)fn->getParent()->getGlobalVariable(prop->str());
    }
    // Inject all properties as full variables
    for(auto prop:ast.getAllProperties()){
        auto prop_name = prop->name;
        vars[prop_name->str()] = (llvm::AllocaInst*)fn->getParent()->getGlobalVariable(prop->str());
    }

    // Inject the args as variables
    for (unsigned int i = 0; i < fn->arg_size(); i++)
    {
        auto arg = fn->getArg(i);
        auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "param " + arg->getName().str());
        builder()->CreateStore(arg, alloc);
        vars[arg->getName().str()] = alloc;
    }

    // Inject the var-args as a known-length list
    if (method->sig->isVariadic())
    {
        auto varParam = method->sig->params[method->sig->params.size() - 1];
        auto varParamType = (ListType *)varParam->type;
        auto elementType = varParamType->elements->getLLType();
        auto one = builder()->getInt32(1);
        auto lenptr = vars["+vararg_count"];
        varParamType->size = Ident::get("+vararg_count");
        auto len = builder()->CreateLoad(lenptr->getType()->getPointerElementType(), lenptr, "varargs_len");

        auto list = builder()->CreateAlloca(elementType, len, "varargs_list");
        auto init = builder()->CreateAlloca(elementType->getPointerTo());
        builder()->CreateStore(list, init);
        vars[varParam->name->str()] = init;
        pushf(*method, (Statement *)list);

        auto i32 = Primitive::get(PR_int32)->getLLType();
        auto i8p = (new TPtr(Primitive::get(PR_int8)))->getLLType();
        auto st = llvm::StructType::create(*llctx(), {i32, i32, i8p, i8p});
        auto alloc = builder()->CreateAlloca(st, nullptr, "var_args_obj");
        // Tracker / i
        auto tracker = builder()->CreateAlloca(i32, nullptr, "i");
        builder()->CreateStore(builder()->getInt32(0), tracker);
        auto cast = builder()->CreateBitCast(alloc, i8p);
        builder()->CreateUnaryIntrinsic(llvm::Intrinsic::vastart, cast);

        // Create a loop to iter over each arg, and push it into the list
        auto eval = llvm::BasicBlock::Create(*llctx(), "while_eval", fn);
        auto body = llvm::BasicBlock::Create(*llctx(), "while_body", fn);
        auto cont = llvm::BasicBlock::Create(*llctx(), "while_cont", fn);

        builder()->CreateBr(eval);
        builder()->SetInsertPoint(eval);

        // Generate the Evaluator
        auto loaded_i = builder()->CreateLoad(tracker->getType()->getPointerElementType(), tracker);
        auto isSmaller = builder()->CreateICmpSLT(loaded_i, len);
        builder()->CreateCondBr(isSmaller, body, cont);
        builder()->SetInsertPoint(body);

        // get the value and set it in the list
        auto value = builder()->CreateVAArg(alloc, elementType);
        auto listPtr = builder()->CreateGEP(list->getType()->getPointerElementType(), list, loaded_i);
        builder()->CreateStore(value, listPtr);

        // Increment and jump out
        auto inc = builder()->CreateAdd(loaded_i, one);
        builder()->CreateStore(inc, tracker);
        builder()->CreateBr(eval);
        builder()->SetInsertPoint(cont);
    }
    return vars;
}
std::unique_ptr<llvm::Module> generateModule(Module &mod, std::vector<TopLevelExpression *> injectedDefinitions, CompilationUnit& ast)
{
    auto llmod = std::make_unique<llvm::Module>(mod.name->str(), *llctx());

    auto m = llmod.get();
    for (auto d : injectedDefinitions)
    {
        if (auto method = dynamic_cast<MethodPredeclaration*>(d))
        {
            make_fn(*method->of->sig, m);
        }
        if (auto prop = dynamic_cast<PropertyPredeclaration*>(d))
        {
            if(prop->of->instance_access)continue;
            make_global(prop->of, m);
        }
    }
    for (auto child : mod)
    {
        if (instanceof <Method>(child))
        {
            auto method = (Method *)child;
            if (!method->generate())
                continue;
            auto fname = method->sig->sourcename();
            auto fn = llmod->getFunction(fname);
            if (fn == nullptr)
                error("Function " + fname + " could not be found");
            if (!method->size())
                continue;
            auto entry_block = llvm::BasicBlock::Create(*llctx(), "entry_block", fn);

            builder()->SetInsertPoint(entry_block);
            gen_src(*method, fn, inject_vars(fn, ast, method));
            if (fn->getReturnType()->isVoidTy())
                builder()->CreateRetVoid();
            continue;
        }
        else if (instanceof <Property>(child))
        {
            continue;
        }
        else
            error("Failed to generate module member");
    }
    return llmod;
}

llvm::Module *Codegen::codegen(CompilationUnit &ast)
{
    auto rootmod = new llvm::Module("Quinoa Program", *llctx());
    std::vector<TopLevelExpression *> defs;
    // Generate all of the modules, and link them into the root module "Quinoa Program"
    for (auto unit : ast)
    {
        if (instanceof <Module>(unit))
        {
            auto mod = (Module *)unit;

            // If the module has generic params,
            // just skip it out
            if (mod->generics.size())
            {
                continue;
            }
            auto llmodptr = generateModule(*mod, defs, ast);
            llvm::Linker::linkModules(*rootmod, std::move(llmodptr));
        }
        else if (instanceof <Entrypoint>(unit))
        {
            Logger::debug("gen entrypoint");
            auto entry = (Entrypoint *)unit;
            auto tgt = entry->calls->sourcename();
            auto fn = rootmod->getFunction(tgt);
            if (fn == nullptr)
                error("Failed to locate entrypoint function '" + tgt + "'");

            MethodSignature entrySig;
            std::vector<Param *> params;
            params.push_back(new Param(Primitive::get(PR_int32), Ident::get("argc")));
            params.push_back(new Param(new TPtr(Primitive::get(PR_string)), Ident::get("argv")));
            entrySig.name = new ModuleMemberRef(nullptr, Ident::get("main"));
            entrySig.nomangle = true;
            entrySig.returnType = Primitive::get(PR_int32);
            entrySig.params = Block<Param>(params);
            auto efn = make_fn(entrySig, rootmod, llvm::Function::LinkageTypes::ExternalLinkage, false);

            auto block = llvm::BasicBlock::Create(*llctx(), "main_entry", efn);
            builder()->SetInsertPoint(block);
            auto argc = efn->getArg(0);
            auto argv = efn->getArg(1);
            auto retVal = builder()->CreateCall(fn, {argc, argv});
            builder()->CreateRet(retVal);
        }
        else if (
            instanceof <MethodPredeclaration>(unit) ||
            instanceof <PropertyPredeclaration>(unit))
        {
            defs.push_back(unit);
        }
        else
            error("An Unknown top-level entity was encountered");
    }
    return rootmod;
}