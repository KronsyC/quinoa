#pragma once
#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_globals.h"
#include "../token/token.h"
#include "../../lib/list.h"

#include "./reference.hh"
enum PrimitiveType
{
    PRIMITIVES_ENUM_MEMBERS
};
static std::map<PrimitiveType, std::string> primitive_names{PRIMITIVES_ENUM_NAMES};
static std::map<TokenType, PrimitiveType> primitive_mappings{PRIMITIVES_ENUM_MAPPINGS};

class Primitive;
class Ptr;
class ListType;
class TypeRef;

class Type : public ANode
{
public:
    virtual llvm::Type *llvm_type() = 0;
    virtual std::shared_ptr<Type> pointee() = 0;
    virtual std::string str() = 0;
    template <class T>
    T *get()
    {
        static_assert(std::is_base_of<Type, T>(), "You can only convert a type to a subtype");
        return dynamic_cast<T *>(this->drill());
    }

    virtual bool operator==(Type &compare) = 0;

    virtual int distance_from(Type &target) = 0;

    virtual Type *drill() = 0;

    virtual llvm::Constant *default_value(llvm::Type *expected) = 0;

protected:
    /**
     * Helper method to instantiate a type onto the heap
     * types take heavy inspiration from llvms type system
     * in that they are 'singletons' and can be directly
     * compared for equality
     *
     * achieves this singleton behavior by maintaining an
     * internal cache of objects to their heap equivalent
     */
    template <class T>
    static std::shared_ptr<T> create_heaped(T obj)
    {
        static_assert(std::is_base_of<Type, T>(), "You may only create a heap allocation of a type derivative");
        // O(n) cache mechanism
        // so we do not have to impl a
        // custom hashing fn for each type
        // TODO: optimize
        static std::vector<T> keys;
        static std::vector<std::shared_ptr<T>> values;
        auto idx = indexof(keys, obj);
        if (idx == -1)
        {
            auto alloc = std::make_shared<T>(obj);
            keys.push_back(obj);
            values.push_back(alloc);
            return alloc;
        }
        return values[idx];
    }
};

class Primitive : public Type
{
protected:
    Type *drill()
    {
        return this;
    }

public:
    PrimitiveType kind;
    Primitive(PrimitiveType kind)
    {
        this->kind = kind;
    }
    std::string str()
    {
        return primitive_names[kind];
    }
    bool is(PrimitiveType kind)
    {
        return this->kind == kind;
    }
    int distance_from(Type &target)
    {
        if (!target.get<Primitive>())
            return -1;
        auto &prim = *(Primitive *)&target;
        if (prim.kind == this->kind)
            return 0;

        if (primitive_group_mappings[prim.kind] == primitive_group_mappings[kind])
            return 1;
        return -1;
    }

    static std::shared_ptr<Primitive> get(PrimitiveType type)
    {
        return create_heaped(Primitive(type));
    }
    std::shared_ptr<Type> pointee()
    {
        std::shared_ptr<Type> ret(nullptr);
        return ret;
    }
    llvm::Type *llvm_type()
    {
#define T(sw, name) \
    case PR_##sw:   \
        return builder()->get##name##Ty();
        switch (kind)
        {
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
        default:
            except(E_INTERNAL, "Failed to generate primitive type");
        }
    }
    bool operator==(Type &against)
    {
        if (auto k = against.get<Primitive>())
        {
            return k->kind == kind;
        }
        return false;
    }
    llvm::Constant *default_value(llvm::Type *expected)
    {
        llvm::Constant *ret = nullptr;
        switch (kind)
        {
        case PR_int8:
        case PR_uint8:
            ret = builder()->getInt8(0);
            break;
        case PR_int16:
        case PR_uint16:
            ret = builder()->getInt16(0);
            break;
        case PR_int32:
        case PR_uint32:
            ret = builder()->getInt32(0);
            break;
        case PR_int64:
        case PR_uint64:
            ret = builder()->getInt64(0);
            break;
        default: except(E_INTERNAL, "Cannot get default value for type: " + str());
        }

        auto op = llvm::CastInst::getCastOpcode(ret, true, expected, true);
        return llvm::ConstantExpr::getCast(op, ret, expected, false);
    }

private:
    static inline std::map<PrimitiveType, std::string> primitive_group_mappings{
        PRIMITIVES_ENUM_GROUPS};
};

class Ptr : public Type
{
protected:
    Type *drill()
    {
        return this;
    }

public:
    Ptr(std::shared_ptr<Type> type)
    {
        this->of = type;
    }
    std::shared_ptr<Type> of;

    std::string str()
    {
        return of->str() + "*";
    }

    llvm::Type *llvm_type()
    {
        return of->llvm_type()->getPointerTo();
    }
    std::shared_ptr<Type> pointee()
    {
        return of;
    }
    static std::shared_ptr<Ptr> get(std::shared_ptr<Type> to)
    {
        return create_heaped(Ptr(to));
    }

    bool operator==(Type &against)
    {
        if (auto k = against.get<Ptr>())
        {
            return k->of == of;
        }
        return false;
    }

    int distance_from(Type &target)
    {
        auto pt = target.get<Ptr>();
        if (!pt)
            return -1;
        return this->of->distance_from(*pt->of);
    }
    llvm::Constant *default_value(llvm::Type *expected){
        return llvm::ConstantExpr::getIntToPtr(builder()->getInt64(0), expected, false);
    }
};

class ListType : public Type
{
protected:
    Type *drill()
    {
        return this;
    }

public:
    std::shared_ptr<Type> of;
    ListType(std::shared_ptr<Type> type)
    {
        this->of = std::move(type);
    }
    std::shared_ptr<Type> pointee()
    {
        return of;
    }

    std::string str()
    {
        return of->str() + "[]";
    }
    static std::shared_ptr<ListType> get(std::shared_ptr<Type> of)
    {
        return create_heaped(ListType(of));
    }
    llvm::Type *llvm_type()
    {
        return of->llvm_type()->getPointerTo();
    }
    bool operator==(Type &against)
    {
        if (auto k = against.get<ListType>())
        {
            return k->of == of;
        }
        return false;
    }

    int distance_from(Type &target)
    {
        auto lt = target.get<ListType>();
        if (!lt)
            return -1;
        return this->of->distance_from(*lt->of);
    }
    llvm::Constant *default_value(llvm::Type *expected){
        except(E_INTERNAL, "default value not implemented for list types");
    }

};

class StructType : public Type
{
public:
    std::map<std::string, std::shared_ptr<Type>> members;
    StructType(std::map<std::string, std::shared_ptr<Type>> members){

        this->members = members;
    }
    Type *drill()
    {
        return this;
    }

    std::string str()
    {
        std::string name = "struct:{";
        bool first = true;
        for(auto member :members){
            if(!first)name+=",";

            name+=member.first + ":" + member.second->str();

            first = false;
        }
        name += "}";
        return name;
    }
    std::shared_ptr<Type> pointee()
    {
        return std::shared_ptr<Type>(nullptr);
    }

    int member_idx(std::string name){
        int i = 0;
        for(auto m : members){
            if(m.first == name)return i;
            i++;
        }
        return -1;
    }

    static std::shared_ptr<StructType> get(std::map<std::string, std::shared_ptr<Type>> members)
    {
        return create_heaped(StructType(members));
    }

    int distance_from(Type &target)
    {
        if(!target.get<StructType>())return -1;
        if(target.get<StructType>() == this)return 0;

        // Distance is equal to that of the highest
        except(E_INTERNAL, "distance between: " + str() + " and " + target.str());
    }
    llvm::Constant *default_value(llvm::Type *expected){
        except(E_INTERNAL, "default value not implemented for struct types");
    }
    bool operator==(Type &against)
    {
        auto st = against.get<StructType>();
        if (!st)return false;
        return st->members == members;
    }

    llvm::Type *llvm_type()
    {
        std::vector<llvm::Type*> member_types;
        for(auto member : members){
            member_types.push_back(member.second->llvm_type());
        }
        return llvm::StructType::get(*llctx(), member_types);
    }
};

class TypeRef : public Type
{
public:
    std::unique_ptr<LongName> name;
    std::shared_ptr<Type> resolves_to;

    Type *drill()
    {
        if (resolves_to)
            return resolves_to->drill();
        return this;
    }
    llvm::Constant *default_value(llvm::Type *expected){
        if(resolves_to)return resolves_to->default_value(expected);
        except(E_INTERNAL, "cannot get default value for unresolved type reference");
    }
    std::string str()
    {
        if (resolves_to)
            return resolves_to->str();
        else
            return "unresolved_t:" + name->str();
    }
    std::shared_ptr<Type> pointee()
    {
        return resolves_to ? resolves_to->pointee() : std::shared_ptr<Type>(nullptr);
    }

    static std::shared_ptr<TypeRef> get(std::unique_ptr<LongName> name)
    {
        // do not cache TypeRefs, as multiple refs may share the same name
        // but refer to an entirely different type depending on context

        auto pt = std::make_shared<TypeRef>();
        pt->name = std::move(name);
        return pt;
    }
    llvm::Type *llvm_type()
    {
        if (resolves_to)
            return resolves_to->llvm_type();
        except(E_UNRESOLVED_TYPE, "Cannot generate llvm value for unresolved type reference: " + name->str());
    }
    bool operator==(Type &against)
    {
        if (!resolves_to)
            return false;
        return *resolves_to == against;
    }

    int distance_from(Type &target)
    {
        if (!resolves_to)
            return -1;
        return resolves_to->distance_from(target);
    }
};
class Generic : public TypeRef
{
};