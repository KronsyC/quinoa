#pragma once
#include "./ast.hh"
#include "../../GenMacro.h"
#include<map>
#include "../token/TokenDef.h"
enum PrimitiveType{
    PRIMITIVES_ENUM_MEMBERS
};

static std::map<TokenType, PrimitiveType> primitive_mappings{
  PRIMITIVES_ENUM_MAPPINGS  
};
static std::map<PrimitiveType, std::string> primitive_names{
    PRIMITIVES_ENUM_NAMES
};
class Primitive:public Type{
private:
    Primitive(PrimitiveType t){
        type = t;
    }
public:
    PrimitiveType type;

    std::string str(){
        return primitive_names[type];
    }
    static Primitive* get(PrimitiveType t){
        static std::map<PrimitiveType, Primitive*> cache;
        auto fetched = cache[t];
        if(fetched == nullptr){
            auto prim = new Primitive(t);
            cache[t] = prim;
            return prim;
        }
        Logger::debug("Fetching Primitive Cached Type");
        return fetched;
    }

    llvm::Type* getLLType(){
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

class CustomType:public Type{
private:
    CustomType(Identifier* refersTo){
        name = refersTo;
    }
public:
    Identifier* name;

    llvm::Type* getLLType(){
        error("Cannot get type for customtype");
        return nullptr;
    }

    static CustomType* get(Identifier* refersTo){
        static std::map<Identifier*, CustomType*> cache;
        auto lookup = cache[refersTo];
        if(lookup == nullptr){
            auto p = CustomType::get(refersTo);
            cache[refersTo] = p;
            return p;
        }
        Logger::debug("Fetching Custom Type from cache");
        return lookup;
    }
};
class TPtr:public Type{
private:
    TPtr(Type* type){
        to = type;
    }
public:
    Type* to;

    std::string str(){
        return to->str()+"*";
    }
    llvm::Type* getLLType(){
        return to->getLLType()->getPointerTo();
    }
    static TPtr* get(Type* t){
        static std::map<Type*, TPtr*> cache;
        auto fetched = cache[t];
        if(fetched == nullptr){
            auto val = new TPtr(t);
            cache[t] = val;
            return val;
        }
        Logger::debug("Fetching Cached TPtr");
        return fetched;
    }
};

class ListType:public Type{
private:
    ListType(Type* eT, Expression* n){
        elements = eT;
        size = n;
    }
public:
    Type* elements;
    Expression* size = nullptr;
    ListType() = default;
    llvm::Type* getLLType(){
        return elements->getLLType()->getPointerTo();
    }
    std::string str(){
        return elements->str()+"[]";
    }
    
    static ListType* get(Type* t, Expression* n=nullptr){
        static std::map<std::pair<Type*, Expression*>, ListType*> cache;
        auto fetched = cache[{t, n}];
        if(fetched == nullptr){
            auto val = new ListType(t, n);
            cache[{t, n}] = val;
            return val;
        }
        return fetched;
    }
};