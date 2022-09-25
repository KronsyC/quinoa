#include "./codegen.hh"
#include "../AST/ast.hh"
#include "../../lib/error.h"
#include <string>
#include <map>
#include "llvm/LinkAllIR.h"
#include "llvm/Linker/Linker.h"
#include "llvm/IR/IRBuilder.h"

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
        return nullptr;
    }
}
llvm::Function *createFunction(Method &f, llvm::Module *mod, llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage)
{

    auto ret = getType(f.returnType);
    auto name = f.name->str();
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
    return i->isIntegerTy(1) | i->isIntegerTy(8) | i->isIntegerTy(16) | i->isIntegerTy(32) | i->isIntegerTy(64);
}
llvm::Constant *createInt(Integer *i, llvm::Type *expected)
{
    // TODO: Make this size-aware
    if (expected == nullptr)
        return builder.getInt32(i->value);
    else
    {
        if (!isInt(expected))
            error("Expected an Integer");
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
llvm::Value *genExpression(Expression *expr, TVars vars, llvm::Type *expectedType = nullptr)
{
    if (instanceof <Integer>(expr))
        return createInt((Integer *)expr, expectedType);
    if (instanceof <String>(expr))
        return createStr((String *)expr, expectedType);
    if (instanceof <Ident>(expr))
    {
        auto e = (Ident *)expr;
        auto var = loadVar(e, vars);
        auto loaded = builder.CreateLoad(var->getType()->getPointerElementType(), var, "loaded var '" + e->str() + "'");
        return loaded;
    }
    if (instanceof <MethodCall>(expr))
    {
        auto call = (MethodCall *)expr;
        auto self = builder.GetInsertBlock()->getParent();
        auto mod = self->getParent();
        auto tgtFn = mod->getFunction(call->target->str());
        if (tgtFn == nullptr)
            tgtFn = mod->getFunction(mod->getName().str() + "." + call->target->str());
        if (tgtFn == nullptr)
            error("Failed to call function '" + call->target->str() + "' (doesn't exist)");
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
void genSource(vector<Statement *> content, llvm::Function *func, TVars vars)
{
    for (auto stm : content)
    {
        if (instanceof <InitializeVar>(stm))
        {
            auto init = (InitializeVar *)stm;
            auto varname = init->varname->str();
            auto type = getType(init->type);
            auto alloca = builder.CreateAlloca(type, nullptr, "var " + varname);

            vars.insert({varname, alloca});
            continue;
        }
        else if (instanceof <Return>(stm))
        {
            auto ret = (Return *)stm;
            auto expr = genExpression(ret->retValue, vars, func->getReturnType());
            builder.CreateRet(expr);
            continue;
        }
        else if (instanceof <Expression>(stm))
        {
            genExpression((Expression *)stm, vars);
            continue;
        }
        // interpret as expression with dumped value
        else
            error("Failed Generate IR for statement");
    }
}
TVars varifyArgs(llvm::Function* fn)
{
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
std::unique_ptr<llvm::Module> generateModule(Module &mod, std::vector<Method> injectedDefinitions)
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

            // auto fn = m->getFunction(method->fullname->str());
            auto fn = m->getFunction(mod.name->str() + "." + method->name->str());
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
    std::vector<Method> defs;
    // Generate all of the modules, and link them into the root module "Quinoa Program"
    for (auto unit : ast.items)
    {
        if (instanceof <Module>(unit))
        {
            auto mod = (Module *)unit;
            auto llmodptr = generateModule(*mod, defs);

            llvm::Linker::linkModules(*rootmod, std::move(llmodptr));
        }
        else if (instanceof <MethodDefinition>(unit))
        {
            auto def = (MethodDefinition *)unit;
            Method fn;
            if (instanceof <Ident>(def->name))
                fn.name = (Ident *)def->name;
            else
                fn.name = ((CompoundIdentifier *)def->name)->last();
            fn.params = def->params;
            fn.returnType = def->returnType;
            defs.push_back(fn);
        }
        else if (instanceof <Entrypoint>(unit))
        {
            auto entry = (Entrypoint *)unit;
            auto tgt = entry->calls;
            auto fn = rootmod->getFunction(tgt->str());
            if (fn == nullptr)
                error("Failed to locate entrypoint function");
            // Construct the entrypoint method
            auto efn = createFunction(*((Method *)entry), rootmod);
            auto block = llvm::BasicBlock::Create(ctx, "main_entry", efn);
            builder.SetInsertPoint(block);
            genSource(entry->items, efn, varifyArgs(efn));
        }
        else
            error("An Unknown top-level entity was encountered");
    }
    return rootmod;
}