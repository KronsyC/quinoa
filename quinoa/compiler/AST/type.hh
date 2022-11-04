#pragma once
#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_globals.h"
enum PrimitiveType { PRIMITIVES_ENUM_MEMBERS };
static std::map<PrimitiveType, std::string> primitive_names { PRIMITIVES_ENUM_NAMES };

class Type: public ANode{
public:
    virtual llvm::Type* llvm_type() = 0;
};


class Primitive: public ANode{
private:
    Primitive(PrimitiveType kind){
        this->kind = kind;
    }
public:
    PrimitiveType kind;

    std::string str(){
        return primitive_names[kind];
    }


    static std::unique_ptr<Primitive> get(PrimitiveType type){
        auto ret = std::make_unique<Primitive>(type);
        return ret;
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
        }
    }
};

class Ptr: public Type{
private:
    Ptr(std::unique_ptr<Type> type){
        this->of = std::move(type);
        
    }
public:
    std::unique_ptr<Type> of;

    
    std::string str(){
        return of->str() + "*";
    }

    llvm::Type* llvm_type(){
        return of->llvm_type()->getPointerTo();
    }

    static std::unique_ptr<Ptr> get(std::unique_ptr<Type> to){
        return std::make_unique<Ptr>(to);
    }
};