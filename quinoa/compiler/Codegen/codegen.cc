#include "./codegen.hh"
#include "../AST/ast.hh"
#include "../../lib/error.h"
#include <string>
#include <map>
#include "llvm/Linker/Linker.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "../../lib/logger.h"
using namespace std;

#define TVars std::map<std::string, llvm::AllocaInst *>

llvm::Type *getType(Type *type)
{
    if (type == nullptr)
        error("Failed to generate a type as a nullptr was provided");
    if (instanceof <Primitive>(type))
    {
        auto prim = (Primitive *)type;
        switch (prim->type)
        {
        case PR_int8:
            return llvm::Type::getInt8Ty(ctx);
        case PR_int16:
            return llvm::Type::getInt16Ty(ctx);
        case PR_int32:
            return llvm::Type::getInt32Ty(ctx);
        case PR_int64:
            return llvm::Type::getInt64Ty(ctx);

        case PR_uint8:
            return llvm::Type::getInt8Ty(ctx);
        case PR_uint16:
            return llvm::Type::getInt16Ty(ctx);
        case PR_uint32:
            return llvm::Type::getInt32Ty(ctx);
        case PR_uint64:
            return llvm::Type::getInt64Ty(ctx);

        case PR_float16:
            return llvm::Type::getHalfTy(ctx);
        case PR_float32:
            return llvm::Type::getFloatTy(ctx);
        case PR_float64:
            return llvm::Type::getDoubleTy(ctx);

        case PR_boolean:
            return llvm::Type::getInt1Ty(ctx);
        case PR_string:
            return llvm::Type::getInt8PtrTy(ctx); // This type is just temporary //TODO: implement string module within the language
        case PR_void:
            return llvm::Type::getVoidTy(ctx);
        default:
            error("Failed to generate primitive for " + to_string(prim->type));
        }
    }
    else if (instanceof <TPtr>(type))
    {
        auto p = (TPtr *)type;
        return getType(p->to)->getPointerTo();
    }
    else
    {
        error("Custom type support is not yet implemented");
    }
        return nullptr;
}
llvm::Function *createFunction(MethodSignature &f, llvm::Module *mod, llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage, bool mangle = true)
{
    auto ret = getType(f.returnType);
    auto name = mangle?f.sourcename():f.fullname()->str();
    vector<llvm::Type *> args;
    for (auto a : f.params)
        args.push_back(getType(a->type));
    auto sig = llvm::FunctionType::get(ret, args, false);
    auto fn = llvm::Function::Create(sig, linkage, name, mod);
    for (int i = 0; i < fn->arg_size(); i++)
        fn->getArg(i)->setName(f.params[i]->name->str());
    return fn;
}
bool isInt(llvm::Type *i)
{
    return i->isIntegerTy(1) || i->isIntegerTy(8) || i->isIntegerTy(16) || i->isIntegerTy(32) || i->isIntegerTy(64);
}
bool isFloat(llvm::Type* i){
    return i->isFloatingPointTy() || i->isHalfTy() || i->isDoubleTy();
}
llvm::Constant *createInt(Integer *i, llvm::Type *expected)
{
    // TODO: Make this size-aware
    if (expected == nullptr)
        return builder.getInt32(i->value);
    else
    {
        if (!isInt(expected)){
            expected->print(llvm::outs());
            error("Expected an Integer type for "+ to_string(i->value));


        }
        return builder.getIntN(expected->getPrimitiveSizeInBits(), i->value);
    }
}
llvm::Constant *createStr(String *s, llvm::Type *expects)
{
    if (expects == nullptr)
        return builder.CreateGlobalStringPtr(s->value, "str");
    else
    {
        if (expects->isPointerTy() && expects->getPointerElementType()->isIntegerTy(8))
            return builder.CreateGlobalStringPtr(s->value, "str");
        error("Failed to generate String");
        return nullptr;
    }
}
llvm::Value *genExpression(Expression *expr, TVars vars, llvm::Type *expectedType);
llvm::Value *loadVar(Identifier *ident, TVars vars)
{
    auto loaded = vars[ident->str()];
    if (loaded == nullptr)
        error("Failed to read variable '" + ident->str() + "'");
    return loaded;
}
llvm::Value *loadVar(Subscript *subscr, TVars vars)
{
    auto name = subscr->tgt;
    auto var = loadVar(name, vars);
    auto varl = builder.CreateLoad(var->getType()->getPointerElementType(), var, "temp load for subscript");
    auto idx = genExpression(subscr->item, vars, nullptr);
    auto loaded = builder.CreateGEP(varl->getType()->getPointerElementType(), varl, idx, "subscript-ptr of '" + name->str() + "'");
    return loaded;
}
llvm::Value* cast(llvm::Value* val, llvm::Type* type){
    if(type == nullptr)return val;
    if(val->getType() == type)return val; 
    if(isInt(type) && isInt(val->getType()))return builder.CreateIntCast(val, type, true);
    if(isFloat(type) && isFloat(val->getType()))return builder.CreateFPCast(val, type);
    error("Failed to cast");
    return nullptr;
}
llvm::Value *genExpression(Expression *expr, TVars vars, llvm::Type *expectedType = nullptr)
{
    if (instanceof <Integer>(expr))
        return createInt((Integer *)expr, expectedType);
    if (instanceof <String>(expr))
        return createStr((String *)expr, expectedType);
    if(instanceof<Boolean>(expr)){
        auto boo = (Boolean*)expr;
        return builder.getInt1(boo->value);
    }
    if (instanceof <Ident>(expr))
    {
        auto e = (Ident *)expr;
        auto var = loadVar(e, vars);
        auto loaded = builder.CreateLoad(var->getType()->getPointerElementType(), var, "loaded var '" + e->str() + "'");
        return cast(loaded, expectedType);
    }
    if (instanceof <MethodCall>(expr))
    {
        auto call = (MethodCall *)expr;
        if(call->target == nullptr)error("Received an unresolved call for "+call->name->str());
        auto mod = builder.GetInsertBlock()->getParent()->getParent();
        auto name = call->target->sourcename();
        auto tgtFn = mod->getFunction(name);
        if (tgtFn == nullptr)
            error("Failed to call function '" + name + "' (doesn't exist)");
        vector<llvm::Value *> params;
        int i = 0;
        for (auto p : call->params)
        {
            auto type = tgtFn->getArg(i)->getType();
            params.push_back(genExpression(p, vars, type));
            i++;
        }

        return builder.CreateCall(tgtFn, params);
    }
    if (instanceof <Subscript>(expr))
    {
        auto sub = loadVar((Subscript *)expr, vars);
        return builder.CreateLoad(sub->getType()->getPointerElementType(), sub, "element of " + ((Subscript *)expr)->tgt->str());
    }
    if (instanceof <BinaryOperation>(expr))
    {
        auto op = (BinaryOperation *)expr;
        auto left = op->left;
        auto right = op->right;
        auto r = genExpression(right, vars, expectedType);
        if (op->op == BIN_assignment)
        {
            llvm::Value *var;
            if (instanceof <Identifier>(left))
                var = loadVar((Identifier *)left, vars);
            else if (instanceof <Subscript>(left))
                var = loadVar((Subscript *)left, vars);
            else
                error("Failed to load Variable");
            builder.CreateStore(r, var);
            return r;
        }
        auto l = genExpression(left, vars, expectedType);
        switch (op->op)
        {
        case BIN_plus:
            return builder.CreateAdd(l, r);
        case BIN_minus:
            return builder.CreateSub(l, r);
        case BIN_star:
            return builder.CreateMul(l, r);
        case BIN_slash:
            return builder.CreateSDiv(l, r);
        default:
        {
            error("Failed to generate binary operation for op " + std::to_string(op->op));
        }
        }
    }
    error("Failed to Generate Expression");
    return nullptr;
}


struct ControlFlowInfo{
    // The block to jump to after the `break` action is invoked
    llvm::BasicBlock* breakTo;

    // The block to jump to after the `continue` action is invoked
    llvm::BasicBlock* continueTo;

    // The block to jump to after the `fallthrough` action is invoked
    llvm::BasicBlock* fallthroughTo;
    
    // The block to break to after the inner scope is executed
    llvm::BasicBlock* exitBlock;
};

void genSource(vector<Statement *> content, llvm::Function *func, TVars vars, ControlFlowInfo cfi = {})
{
    for (auto stm : content)
    {
        if(!stm->active)continue;
        if (instanceof <InitializeVar>(stm))
        {
            auto init = (InitializeVar *)stm;
            auto varname = init->varname->str();
            auto type = getType(init->type);
            auto alloca = builder.CreateAlloca(type, nullptr, "var " + varname);

            vars.insert({varname, alloca});
        }
        else if(instanceof<WhileCond>(stm)){
            auto loop = (WhileCond*)stm;
            auto cond = genExpression(loop->cond, vars, getType(new Primitive(PR_boolean)));

            auto evaluatorBlock = llvm::BasicBlock::Create(ctx, "while_evaluator", func);
            auto runBlock = llvm::BasicBlock::Create(ctx, "while_content", func);
            auto continuationBlock = llvm::BasicBlock::Create(ctx, "while_continuation", func);

            ControlFlowInfo cf = cfi;
            cf.breakTo = continuationBlock;
            cf.continueTo = evaluatorBlock;
            cf.exitBlock = continuationBlock;

            // Set up the evaluator
            builder.CreateBr(evaluatorBlock);      
            builder.SetInsertPoint(evaluatorBlock);
            builder.CreateCondBr(cond, runBlock, continuationBlock);    

            // Set up the content
            builder.SetInsertPoint(runBlock);
            genSource(loop->items, func, vars, cf);
            builder.CreateBr(evaluatorBlock);

            // generate the continuation
            builder.SetInsertPoint(continuationBlock);  
        }
        else if (instanceof <Return>(stm))
        {
            auto ret = (Return *)stm;
            auto expr = genExpression(ret->retValue, vars, func->getReturnType());
            builder.CreateRet(expr);
        }
        // interpret as expression with dumped value
        else if (instanceof <Expression>(stm))
        {
            genExpression((Expression *)stm, vars);
        }
        else
            error("Failed Generate IR for statement");
    }
}
TVars varifyArgs(llvm::Function* fn)
{
    if(fn == nullptr)error("Cannot varify the args of a null function", true);
    TVars vars;

    // Inject the args as variables
    for (int i = 0; i < fn->arg_size(); i++)
    {
        auto arg = fn->getArg(i);
        auto alloc = builder.CreateAlloca(arg->getType(), nullptr, "param " + arg->getName().str());
        builder.CreateStore(arg, alloc);
        vars[arg->getName().str()] = alloc;
    }
    return vars;
}
std::unique_ptr<llvm::Module> generateModule(Module &mod, std::vector<MethodSignature> injectedDefinitions)
{
    auto llmod = std::make_unique<llvm::Module>(mod.name->str(), ctx);

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
            if(fn==nullptr)error("Function " + fname +" could not be found");
            auto entry_block = llvm::BasicBlock::Create(ctx, "entry_block", fn);

            builder.SetInsertPoint(entry_block);
            genSource(method->items, fn, varifyArgs(fn));
            if (fn->getReturnType()->isVoidTy())
                builder.CreateRetVoid();
            continue;
        }
        else
            error("Failed to do the stuff");
    }
    return std::move(llmod);
}

llvm::Module *Codegen::codegen(CompilationUnit &ast)
{
    auto rootmod = new llvm::Module("Quinoa Program", ctx);
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
        else if (instanceof<Entrypoint>(unit))
        {
            auto entry = (Entrypoint *)unit;
            auto tgt = entry->calls->sourcename();
            auto fn = rootmod->getFunction(tgt);
            if (fn == nullptr)
                error("Failed to locate entrypoint function '"+tgt+"'");
            // Construct the entrypoint method
            auto efn = createFunction(*entry->sig, rootmod, llvm::Function::LinkageTypes::ExternalLinkage, false);
            auto block = llvm::BasicBlock::Create(ctx, "main_entry", efn);
            builder.SetInsertPoint(block);
            genSource(entry->items, efn, varifyArgs(efn));
        }
        else if(instanceof<MethodPredeclaration>(unit)){
            auto dec = (MethodPredeclaration*)unit;
            defs.push_back(*dec->sig);
        }
        else
            error("An Unknown top-level entity was encountered");
    }
    return rootmod;
}