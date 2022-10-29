#pragma once
#include "../../GenMacro.h"
#include <map>
#include "../token/TokenDef.h"
enum PrimitiveType
{
    PRIMITIVES_ENUM_MEMBERS
};

static std::map<TokenType, PrimitiveType> primitive_mappings{
    PRIMITIVES_ENUM_MAPPINGS};
static std::map<PrimitiveType, std::string> primitive_names{
    PRIMITIVES_ENUM_NAMES};
class Primitive : public Type
{
private:
    Primitive(PrimitiveType t)
    {
        type = t;
    }

public:
    std::vector<Statement *> flatten()
    {
        return {this};
    }
    PrimitiveType type;
    Primitive *primitive()
    {
        return this;
    }
    std::string str()
    {
        return primitive_names[type];
    }
    Type *drill() { return this; }
    Type *copy(SourceBlock *ctx)
    {
        return this;
    }
    static Primitive *get(PrimitiveType t)
    {
        static std::map<PrimitiveType, Primitive *> cache;
        auto fetched = cache[t];
        if (fetched == nullptr)
        {
            auto prim = new Primitive(t);
            cache[t] = prim;
            return prim;
        }
        return fetched;
    }
    bool is(PrimitiveType t)
    {
        return this->type == t;
    }
    Primitive *getMutual(Primitive *w, bool secondPass = false)
    {
        if (this == w)
            return this;
        if (is(PR_int8) && w->is(PR_int16))
            return w;

        if (is(PR_int8) && w->is(PR_int32))
            return w;
        if (is(PR_int16) && w->is(PR_int32))
            return w;

        if (is(PR_int8) && w->is(PR_int64))
            return w;
        if (is(PR_int16) && w->is(PR_int64))
            return w;
        if (is(PR_int32) && w->is(PR_int64))
            return w;

        if (secondPass)
            error("Failed to generate mutual primitive type for " + str() + " and " + w->str());
        return w->getMutual(this, true);
    }

    std::pair<Type *, Type *> find_mismatch(Type *against)
    {
        if (this != against->drill())
            return {this, against->drill()};
        return {nullptr, nullptr};
    }
    llvm::Type *getLLType()
    {
        switch (type)
        {
        case PR_int8:
            return llvm::Type::getInt8Ty(*llctx());
        case PR_int16:
            return llvm::Type::getInt16Ty(*llctx());
        case PR_int32:
            return llvm::Type::getInt32Ty(*llctx());
        case PR_int64:
            return llvm::Type::getInt64Ty(*llctx());

        case PR_uint8:
            return llvm::Type::getInt8Ty(*llctx());
        case PR_uint16:
            return llvm::Type::getInt16Ty(*llctx());
        case PR_uint32:
            return llvm::Type::getInt32Ty(*llctx());
        case PR_uint64:
            return llvm::Type::getInt64Ty(*llctx());

        case PR_float16:
            return llvm::Type::getHalfTy(*llctx());
        case PR_float32:
            return llvm::Type::getFloatTy(*llctx());
        case PR_float64:
            return llvm::Type::getDoubleTy(*llctx());

        case PR_boolean:
            return llvm::Type::getInt1Ty(*llctx());
        case PR_void:
            return llvm::Type::getVoidTy(*llctx());
        case PR_string:
            return llvm::Type::getInt8PtrTy(*llctx());
        default:
            error("Failed to generate primitive for " + std::to_string(type));
        }
        return nullptr;
    }
};
class CustomType : public Type
{
public:
    Type *refersTo = nullptr;
    Block<Type> type_args;
    Identifier *name;

    Type *copy(SourceBlock *ctx)
    {
        auto ct = new CustomType(name);
        ct->ctx = ctx;
        if (refersTo)
        {
            ct->refersTo = refersTo->copy(ctx);
        }
        for (auto a : type_args)
        {
            ct->type_args.push_back(a->copy(ctx));
        }
        return ct;
    }
    CustomType(Identifier *refersTo)
    {
        name = refersTo;
    }
    CustomType *custom()
    {
        return this;
    }
    Type *drill()
    {
        if (refersTo)
            return refersTo->drill();
        return this;
    }

    llvm::Type *getLLType()
    {
        if (refersTo)
            return refersTo->getLLType();
        error("Cannot get type for unresolved type reference " + name->str(), true);
        return nullptr;
    }
    std::string str()
    {
        if (refersTo)
        {
            auto child = refersTo->str();
            return child;
        }
        error("Cannot get name for unresolved type reference: " + name->str(), true);
        return nullptr;
    }

    std::vector<Statement *> flatten()
    {

        std::vector<Statement *> ret = {this};
        if (name)
            for (auto m : name->flatten())
                ret.push_back(m);

        for (auto ta : type_args)
            for (auto f : ta->flatten())
                ret.push_back(f);

        if (refersTo){
            for (auto t : refersTo->drill()->flatten()){
                ret.push_back(t);

            }
        }


        return ret;
    }
};

class Generic : public CustomType
{
public:
    Type *constraint = nullptr;

    Generic(Ident *name, Type *constraint = nullptr)
        : CustomType(name)
    {
        this->constraint = constraint;
    }
    Type *copy(SourceBlock *ctx)
    {
        auto gen = new Generic(*this);
        gen->ctx = ctx;
        if (constraint)
            gen->constraint = constraint->copy(ctx);
        if (refersTo)
            gen->refersTo = refersTo->copy(ctx);
        return gen;
    }
    Type *drill()
    {
        if (refersTo)
            return refersTo;
        return this;
    }
    std::string str()
    {
        if (refersTo)
            return refersTo->str();
        return "G_" + name->str();
    }
    Generic *generic()
    {
        return this;
    }
};

class ModuleRef;
class ModuleType : public Type
{
public:
    ModuleRef *ref;
    ModuleType* mod(){
        return this;
    }
    ModuleType(ModuleRef *ref)
    {
        this->ref = ref;
    }

    std::string str()
    {
        return ref->str();
    }

    std::vector<Statement *> flatten()
    {
        return {this};
    }
    llvm::Type *getLLType()
    {
        error("Cannot get lltype of module reference", true);
        return nullptr;
    }
        Type* drill(){
        return this;
    }
};

class TPtr : public Type
{
public:
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : to->flatten())
            ret.push_back(m);
        return ret;
    }
    TPtr(Type *type)
    {
        to = type;
    }
    Type *copy(SourceBlock *ctx)
    {
        auto pt = new TPtr(to->copy(ctx));
        pt->ctx = ctx;
        return pt;
    }
    TPtr *ptr()
    {
        return this;
    }
    Type *to = nullptr;
    Type *drill()
    {
        return this;
    }
    std::string str()
    {
        return to->str() + "-p";
    }
    llvm::Type *getLLType()
    {
        return to->getLLType()->getPointerTo();
    }
};
class Constant;
class ListType : public Type
{
public:
    ListType *list()
    {
        return this;
    }
    Type *copy(SourceBlock *ctx)
    {
        auto lt = new ListType(
            elements->copy(ctx),
            size);
        lt->ctx = ctx;
        return lt;
    }
    Type *elements;
    Expression *size = nullptr;
    ListType() = default;
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : elements->flatten())
            ret.push_back(m);
        if(size)for(auto ex:size->flatten())ret.push_back(ex);
        return ret;
    }
    Type *drill()
    {
        return this;
    }
    llvm::Type *getLLType()
    {
        return elements->getLLType()->getPointerTo();
    }
    std::string str()
    {
        return elements->str() + "[]";
    }
    bool isStatic()
    {
        return instanceof <Constant>(size);
    }
    ListType(Type *eT, Expression *n = nullptr)
    {
        elements = eT;
        size = n;
    }
};
class ModuleInstanceType : public Type
{
public:
    ModuleInstanceType* inst(){
        return this;
    }
    CustomType* of = nullptr;

    ModuleInstanceType* copy(SourceBlock* ctx){
        auto inst = new ModuleInstanceType(of);
        return inst;
    }
    ModuleInstanceType(CustomType* of){
        this->of = of;
    }
    llvm::StructType* getLLType(){
        auto of = this->of->drill();
        if(!instanceof<ModuleType>(of))error("Module Instance must refer to module type");
        auto base_mod_ref = ((ModuleType*)of)->ref;
        if(!base_mod_ref->refersTo)error("Module Instance must refer to a resolved module type");
        auto mod = base_mod_ref->refersTo;
        auto typ = structify(mod);
        return typ;
    }
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : of->flatten())
            ret.push_back(m);
        return ret;
    }
    Type* drill(){
        return this;
    }

    std::string str(){
        return "ins_"+of->str();
    }

    size_t getMemberIdx(std::string name){
        // auto typ = this->getLLType();
        return 0;
    }
};