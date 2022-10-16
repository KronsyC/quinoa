#pragma once
#include "./ast.hh"
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
    Type* copy(){
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
    llvm::Type *getLLType()
    {
        switch (type)
        {
        case PR_int8:
            return llvm::Type::getInt8Ty(*ctx());
        case PR_int16:
            return llvm::Type::getInt16Ty(*ctx());
        case PR_int32:
            return llvm::Type::getInt32Ty(*ctx());
        case PR_int64:
            return llvm::Type::getInt64Ty(*ctx());

        case PR_uint8:
            return llvm::Type::getInt8Ty(*ctx());
        case PR_uint16:
            return llvm::Type::getInt16Ty(*ctx());
        case PR_uint32:
            return llvm::Type::getInt32Ty(*ctx());
        case PR_uint64:
            return llvm::Type::getInt64Ty(*ctx());

        case PR_float16:
            return llvm::Type::getHalfTy(*ctx());
        case PR_float32:
            return llvm::Type::getFloatTy(*ctx());
        case PR_float64:
            return llvm::Type::getDoubleTy(*ctx());

        case PR_boolean:
            return llvm::Type::getInt1Ty(*ctx());
        case PR_string:
            return llvm::Type::getInt8PtrTy(*ctx()); // This type is just temporary //TODO: implement string module within the language
        case PR_void:
            return llvm::Type::getVoidTy(*ctx());
        default:
            error("Failed to generate primitive for " + std::to_string(type));
        }
        return nullptr;
    }
};
class CustomType : public Type
{
public:
    Type* copy(){
        auto ct = new CustomType(name);
        if(refersTo){
            auto copy = refersTo->copy();
            ct->refersTo = refersTo->copy();
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
    Type* drill(){
        if(refersTo)return refersTo->drill();
        return this;
    }
    Type *refersTo;
    Identifier *name;

    llvm::Type *getLLType()
    {
        if (refersTo)
            return refersTo->getLLType();
        error("Cannot get type for unresolved type reference", true);
        return nullptr;
    }
    std::string str()
    {
        if (refersTo){
            auto child = refersTo->str();
            return child;
        }
        error("Cannot get name for unresolved type reference");
        return nullptr;
    }
};

class Generic:public CustomType{
public:
    Type* constraint;

    Generic(Ident* name, Type* constraint=nullptr)
    :CustomType(name)
    {
        this->constraint = constraint;
    }
    Type* copy(){
        auto gen = new Generic(*this);
        if(constraint)gen->constraint = constraint->copy();
        if(refersTo)gen->refersTo = refersTo->copy();
        return gen;
    }
    Type* drill(){
        if(refersTo)return refersTo;
        return this;
    }
    std::string str(){
        if(refersTo)return refersTo->str();
        return "G_"+name->str();
    }
    Generic* generic(){
        return this;
    }
};

class TPtr : public Type
{
public:
    TPtr(Type *type)
    {
        to = type;
    }
    Type* copy(){
        auto pt = new TPtr(to->copy());
        return pt;
    }
    TPtr *ptr()
    {
        return this;
    }
    Type *to;
    Type* drill(){
        return this;
    }
    std::string str()
    {
        return to->str() + "*";
    }
    llvm::Type *getLLType()
    {
        return to->getLLType()->getPointerTo();
    }
};
class Constant;
class ListType : public Type
{
private:


public:
    ListType *list()
    {
        return this;
    }
    Type* copy(){
        auto lt = new ListType(
            elements->copy(),
            size
        );
        return lt;        
    }
    Type *elements;
    Expression *size = nullptr;
    ListType() = default;
    Type* drill(){
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
    ListType(Type *eT, Expression *n=nullptr)
    {
        elements = eT;
        size = n;
    }
};
