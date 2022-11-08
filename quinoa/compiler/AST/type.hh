#pragma once
#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_globals.h"
#include "../token/token.h"
enum PrimitiveType { PRIMITIVES_ENUM_MEMBERS };
static std::map<PrimitiveType, std::string> primitive_names{ PRIMITIVES_ENUM_NAMES };
static std::map<TokenType, PrimitiveType> primitive_mappings{PRIMITIVES_ENUM_MAPPINGS};

class Primitive;
class Ptr;
class ListType;
class TypeRef;

class Type: public ANode{
public:
    virtual llvm::Type* llvm_type() = 0;
    virtual Type* pointee() = 0;
    virtual std::string str() = 0;

    template<class T>
    T* get(){
        static_assert(std::is_base_of<Type, T>(), "You can only convert a type to a subtype");
        return dynamic_cast<T*>(drill());
    }

protected:
    virtual Type* drill() = 0;

};


class Primitive: public Type{
protected:
    Type* drill(){
        return this;
    }
public:
    PrimitiveType kind;
    Primitive(PrimitiveType kind){
        this->kind = kind;
    }
    std::string str(){
        return primitive_names[kind];
    }


    static std::unique_ptr<Primitive> get(PrimitiveType type){
        auto ret = std::make_unique<Primitive>(type);
        return ret;
    }
    Type* pointee(){
        return nullptr;
    }
    llvm::Type* llvm_type(){
        #define T(name) return builder()->get##name##Ty();
        switch(kind){
            case PR_int8: T(Int8)
            case PR_int16: T(Int16)
            case PR_int32: T(Int32)
            case PR_int64: T(Int64)

            case PR_uint8: T(Int8)
            case PR_uint16: T(Int16)
            case PR_uint32: T(Int32)
            case PR_uint64: T(Int64)

            case PR_float16: T(Half)
            case PR_float32: T(Float)
            case PR_float64: T(Double)

            case PR_boolean: T(Int1)
            case PR_void: T(Void)
            case PR_string: T(Int8Ptr)
            default: except(E_INTERNAL, "Failed to generate primitive type");
        }
    }
};

class Ptr: public Type{
protected:
    Type* drill(){
        return this;
    }
public:

    Ptr(std::unique_ptr<Type> type){
        this->of = std::move(type);
        
    }
    std::unique_ptr<Type> of;

    
    std::string str(){
        return of->str() + "*";
    }

    llvm::Type* llvm_type(){
        return of->llvm_type()->getPointerTo();
    }
    Type* pointee(){
        return of.get();
    }
    static std::unique_ptr<Ptr> get(std::unique_ptr<Type> to){
        return std::make_unique<Ptr>(std::move(to));
    }
};

class ListType: public Type{
protected:
    Type* drill(){
        return this;
    }
public:

    std::unique_ptr<Type> of;
    ListType(std::unique_ptr<Type> type){
        this->of = std::move(type);
    }
    Type* pointee(){
        return of.get();
    }
    llvm::Type* llvm_type(){
        return of->llvm_type()->getPointerTo();
    }
    std::string str(){
        return of->str() + "[]";
    }
    static std::unique_ptr<ListType> get(std::unique_ptr<Type> of){
        return std::make_unique<ListType>(std::move(of));
    }
};
class TypeRef : public Type{
public:
    
};
class Generic : public TypeRef{

};