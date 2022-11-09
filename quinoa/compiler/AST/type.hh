#pragma once
#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_globals.h"
#include "../token/token.h"
#include "../../lib/list.h"
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
    virtual std::shared_ptr<Type> pointee() = 0;
    virtual std::string str() = 0;
    template<class T>
    T* get(){
        static_assert(std::is_base_of<Type, T>(), "You can only convert a type to a subtype");
        return dynamic_cast<T*>(this->drill());
    }

    virtual bool operator==(Type& compare) = 0;



    virtual Type* drill() = 0;
protected:
        /**
     * Helper method to instantiate a type onto the heap
     * types take heavy inspiration from llvm's type system
     * in that they are 'singletons' and can be directly
     * compared for equality
     * 
     * achieves this singleton behavior by maintaining an
     * internal cache of objects to their heap equivalent
    */
    template<class T>
    static std::shared_ptr<T> create_heaped(T obj){
        static_assert(std::is_base_of<Type, T>(), "You may only create a heap allocation of a type derivative");
        // O(n) cache mechanism
        // so we do not have to impl a
        // custom hashing fn for each type
        // TODO: optimize
        static std::vector<T> keys;
        static std::vector<std::shared_ptr<T>> values;
        auto idx = indexof(keys, obj);
        if(idx == -1){
            auto alloc = std::make_shared<T>(obj);
            keys.push_back(obj);
            values.push_back(alloc);
            return alloc;
        }
        return values[idx];
    }
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
    bool is(PrimitiveType kind){
        return this->kind == kind;
    }


    static std::shared_ptr<Primitive> get(PrimitiveType type){
        return create_heaped(Primitive(type));
    }
    std::shared_ptr<Type> pointee(){
        std::shared_ptr<Type> ret(nullptr);
        return ret;
    }
    llvm::Type* llvm_type(){
        #define T(sw, name)case PR_##sw: return builder()->get##name##Ty();
        switch(kind){
            T(int8, Int8)
            T(int16, Int16)
            T(int32, Int32)
            T(int64, Int64)

            T(uint8, Int8)
            T(uint16, Int16)
            T(uint32, Int32) 
            T(uint64, Int64)

            T(float16, Half)
            T(float32, Float)
            T(float64, Double)

            T(boolean, Int1)
            T(void, Void)
            T(string, Int8Ptr)
            default: except(E_INTERNAL, "Failed to generate primitive type");
        }
    }
    bool operator==(Type& against){
        if(auto k = against.get<Primitive>()){
            return k->kind == kind;
        }
        return false;
    }

};

class Ptr: public Type{
protected:
    Type* drill(){
        return this;
    }
public:

    Ptr(std::shared_ptr<Type> type){
        this->of = type;
        
    }
    std::shared_ptr<Type> of;

    
    std::string str(){
        return of->str() + "*";
    }

    llvm::Type* llvm_type(){
        return of->llvm_type()->getPointerTo();
    }
    std::shared_ptr<Type> pointee(){
        return of;
    }
    static std::shared_ptr<Ptr> get(std::shared_ptr<Type> to){
        return create_heaped(Ptr(to));
    }

    bool operator==(Type& against){
        if(auto k = against.get<Ptr>()){
            return k->of == of;
        }
        return false;
    }
};

class ListType: public Type{
protected:
    Type* drill(){
        return this;
    }
public:

    std::shared_ptr<Type> of;
    ListType(std::shared_ptr<Type> type){
        this->of = std::move(type);
    }
    std::shared_ptr<Type> pointee(){
        return of;
    }
    llvm::Type* llvm_type(){
        return of->llvm_type()->getPointerTo();
    }
    std::string str(){
        return of->str() + "[]";
    }
    static std::shared_ptr<ListType> get(std::shared_ptr<Type> of){
        return create_heaped(ListType(of));
    }
    bool operator==(Type& against){
        if(auto k = against.get<ListType>()){
            return k->of == of;
        }
        return false;
    }
};
class TypeRef : public Type{
public:
    
};
class Generic : public TypeRef{

};