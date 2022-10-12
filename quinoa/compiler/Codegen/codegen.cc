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


llvm::Function *createFunction(MethodSignature &f, llvm::Module *mod, llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage, bool mangle = true)
{
    auto ret = f.returnType->getLLType();
    auto name = mangle ? f.sourcename() : f.fullname()->str();
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
    Logger::debug("Created function " + name);
    for (int i = 0; i < fn->arg_size(); i++)
        fn->getArg(i)->setName(f.params[i]->name->str());
    return fn;
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

void genSource(vector<Statement *> content, llvm::Function *func, TVars vars, ControlFlowInfo cfi = {})
{
    for (auto stm : content)
    {
        if (!stm->active)
            continue;
        if (instanceof <InitializeVar>(stm))
        {
            llvm::Value* size = nullptr;
            auto init = (InitializeVar *)stm;
            auto varname = init->varname->str();
            auto type = init->type->getLLType();
            if(instanceof<ListType>(init->type)){
                auto l = (ListType*)init->type;
                if(l->size != nullptr)
                    size = l->size->getLLValue(vars, Primitive::get(PR_int32)->getLLType());
            }
            auto alloca = builder()->CreateAlloca(type, size, "var " + varname);
            vars[varname] = alloca;
        }
        else if (instanceof <WhileCond>(stm))
        {
            auto loop = (WhileCond *)stm;

            auto evaluatorBlock = llvm::BasicBlock::Create(*ctx(), "while_eval", func);
            auto runBlock = llvm::BasicBlock::Create(*ctx(), "while_exec", func);
            auto continuationBlock = llvm::BasicBlock::Create(*ctx(), "while_cont", func);

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
            genSource(loop->items, func, vars, cf);
            builder()->CreateBr(evaluatorBlock);

            // generate the continuation
            builder()->SetInsertPoint(continuationBlock);
        }
        else if(instanceof<IfCond>(stm)){
            auto iff = (IfCond*)stm;


            auto execBlock = llvm::BasicBlock::Create(*ctx(), "if_exec", func);
            auto continuationBlock = iff->returns()?nullptr:llvm::BasicBlock::Create(*ctx(), "if_cont", func);
            auto elseBlock = iff->otherwise==nullptr?continuationBlock:llvm::BasicBlock::Create(*ctx(), "if_else", func);
            ControlFlowInfo cf = cfi;
            cf.exitBlock = continuationBlock;

            auto cond = iff->cond->getLLValue(vars, Primitive::get(PR_boolean)->getLLType());
            builder()->CreateCondBr(cond, execBlock, elseBlock);
            
            builder()->SetInsertPoint(execBlock);
            genSource(iff->does->items, func, vars, cf);
            if(!iff->does->returns())builder()->CreateBr(continuationBlock);

            if(iff->otherwise != nullptr){
                builder()->SetInsertPoint(elseBlock);
                genSource(iff->otherwise->items, func, vars, cf);
                if(!iff->otherwise->returns())builder()->CreateBr(continuationBlock);
            }
            builder()->SetInsertPoint(continuationBlock);
            


        }
        else if(instanceof<ForRange>(stm)){
            auto loop = (ForRange *)stm;

            auto evaluatorBlock = llvm::BasicBlock::Create(*ctx(), "for_eval", func);
            auto incrementBlock = llvm::BasicBlock::Create(*ctx(), "for_inc", func);
            auto runBlock = llvm::BasicBlock::Create(*ctx(), "for_exec", func);
            auto continuationBlock = llvm::BasicBlock::Create(*ctx(), "while_cont", func);

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
            genSource(loop->items, func, vars, cf);
            builder()->CreateBr(incrementBlock);


            builder()->SetInsertPoint(incrementBlock);
            genSource(loop->inc->items, func, vars);
            builder()->CreateBr(evaluatorBlock);
            // generate the continuation
            builder()->SetInsertPoint(continuationBlock);
        }
        else if (instanceof <Return>(stm))
        {
            auto ret = (Return *)stm;
            auto expr = ret->retValue->getLLValue(vars, func->getReturnType());
            builder()->CreateRet(cast(expr, func->getReturnType()));
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
TVars varifyArgs(llvm::Function *fn, Method* sig=nullptr)
{
    if (fn == nullptr)
        error("Cannot varify the args of a null function", true);
    TVars vars;

    // Inject the args as variables
    for (int i = 0; i < fn->arg_size(); i++)
    {
        auto arg = fn->getArg(i);
        auto alloc = builder()->CreateAlloca(arg->getType(), nullptr, "param " + arg->getName().str());
        builder()->CreateStore(arg, alloc);
        vars[arg->getName().str()] = alloc;
    }

    // Inject the var-args as a known-length list
    if(sig!=nullptr && sig->sig->isVariadic()){
        auto varParam = sig->sig->params[sig->sig->params.size()-1];
        auto varParamType = (ListType*)varParam->type;
        auto elementType = varParamType->elements->getLLType();
        auto one = builder()->getInt32(1);
        auto lenptr = vars["+vararg_count"];
        varParamType->size = Ident::get("+vararg_count");
        auto len = builder()->CreateLoad(lenptr->getType()->getPointerElementType(), lenptr, "varargs_len");
        
        auto list = builder()->CreateAlloca(elementType, len, "varargs_list");
        auto init = builder()->CreateAlloca(elementType->getPointerTo());
        builder()->CreateStore(list, init);
        vars[varParam->name->str()] = init;
        Logger::debug("varargs: " + varParam->name->str());
        pushf(sig->items, (Statement*)list);

        auto i32 = Primitive::get(PR_int32)->getLLType();
        auto i8p = TPtr::get(Primitive::get(PR_int8))->getLLType();
        // auto st = llvm::StructType::create(i32, i32, i8ptr, i8ptr);
        auto st = llvm::StructType::create(*ctx(), {i32, i32, i8p, i8p});
        auto alloc = builder()->CreateAlloca(st, nullptr, "var_args_obj");
        // Tracker / i
        auto tracker = builder()->CreateAlloca(i32, nullptr, "i");
        builder()->CreateStore(builder()->getInt32(0), tracker);
        auto cast = builder()->CreateBitCast(alloc, i8p);
        builder()->CreateUnaryIntrinsic(llvm::Intrinsic::vastart, cast);


        // Create a loop to iter over each arg, and push it into the list
        auto eval = llvm::BasicBlock::Create(*ctx(), "while_eval", fn);
        auto body = llvm::BasicBlock::Create(*ctx(), "while_body", fn);
        auto cont = llvm::BasicBlock::Create(*ctx(), "while_cont", fn);

        builder()->CreateBr(eval);
        builder()->SetInsertPoint(eval);

        // Generate the Evaluator
        auto loaded_i = builder()->CreateLoad(tracker->getType()->getPointerElementType(), tracker);
        auto isSmaller = builder()->CreateICmpSLT(loaded_i, len);
        builder()->CreateCondBr(isSmaller, body, cont);
        builder()->SetInsertPoint(body);

        // get the value and set it in the list
        auto value = builder()->CreateVAArg(alloc, elementType);
        auto listPtr = builder()->CreateGEP(list->getType()->getPointerElementType(),list,loaded_i);
        builder()->CreateStore(value, listPtr);

        // Increment and jump out
        auto inc = builder()->CreateAdd(loaded_i, one);
        builder()->CreateStore(inc, tracker);
        builder()->CreateBr(eval);
        builder()->SetInsertPoint(cont);
        
    }
    return vars;
}
std::unique_ptr<llvm::Module> generateModule(Module &mod, std::vector<MethodSignature> injectedDefinitions)
{
    auto llmod = std::make_unique<llvm::Module>(mod.name->str(), *ctx());

    auto m = llmod.get();
    for (auto d : injectedDefinitions)
    {
        createFunction(d, m);
    }
    for (auto child : mod.items)
    {
        if (instanceof <Method>(child))
        {
            auto method = (Method *)child;
            auto fname = method->sig->sourcename();
            auto fn = llmod->getFunction(fname);
            if (fn == nullptr)
                error("Function " + fname + " could not be found");
            if (!method->items.size())
                continue;
            auto entry_block = llvm::BasicBlock::Create(*ctx(), "entry_block", fn);

            builder()->SetInsertPoint(entry_block);
            genSource(method->items, fn, varifyArgs(fn, method));
            if (fn->getReturnType()->isVoidTy())
                builder()->CreateRetVoid();
            continue;
        }
        else
            error("Failed to do the stuff");
    }
    return std::move(llmod);
}

llvm::Module *Codegen::codegen(CompilationUnit &ast)
{
    auto rootmod = new llvm::Module("Quinoa Program", *ctx());
    std::vector<MethodSignature> defs;
    // Generate all of the modules, and link them into the root module "Quinoa Program"
    for (auto unit : ast.items)
    {
        if (instanceof <Module>(unit))
        {
            auto mod = (Module *)unit;
            auto llmodptr = generateModule(*mod, defs);

            llvm::Linker::linkModules(*rootmod, std::move(llmodptr));
        }
        else if (instanceof <Entrypoint>(unit))
        {
            auto entry = (Entrypoint *)unit;
            auto tgt = entry->calls->sourcename();
            auto fn = rootmod->getFunction(tgt);
            if (fn == nullptr)
                error("Failed to locate entrypoint function '" + tgt + "'");
            // Construct the entrypoint method
            auto efn = createFunction(*entry->sig, rootmod, llvm::Function::LinkageTypes::ExternalLinkage, false);
            auto block = llvm::BasicBlock::Create(*ctx(), "main_entry", efn);
            builder()->SetInsertPoint(block);
            genSource(entry->items, efn, varifyArgs(efn, nullptr));
        }
        else if (instanceof <MethodPredeclaration>(unit))
        {
            auto dec = (MethodPredeclaration *)unit;
            defs.push_back(*dec->sig);
        }
        else
            error("An Unknown top-level entity was encountered");
    }
    return rootmod;
}