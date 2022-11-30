#pragma once

#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_utils.h"
#include "../token/token.h"
#include "../../lib/list.h"
#include "./reference.hh"

enum PrimitiveType {
    PRIMITIVES_ENUM_MEMBERS
};
static std::map <PrimitiveType, std::string> primitive_names{PRIMITIVES_ENUM_NAMES};
static std::map <TokenType, PrimitiveType> primitive_mappings{PRIMITIVES_ENUM_MAPPINGS};


class Primitive;

class Ptr;

class ListType;

class DynListType;

class TypeRef;

class ReferenceType;

class Generic;

LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat = true );

class Type : public ANode {
public:
    LLVMType llvm_type() {
        auto ll_type = internal_llvm_type();
        if(!self)except(E_INTERNAL, "(bug) self property is null");
        return LLVMType{ll_type, self};
    }

    virtual llvm::Type *internal_llvm_type() = 0;

    virtual std::shared_ptr <Type> pointee() = 0;

    virtual std::string str() = 0;

    template<class T>
    T *get() {
        static_assert(std::is_base_of<Type, T>(), "You can only convert a type to a subtype");
        return dynamic_cast<T *>(this->drill());
    }

    virtual bool operator==(Type &compare) = 0;

    virtual int distance_from(Type &target) = 0;

    virtual Type *drill() = 0;

    virtual llvm::Constant *default_value(LLVMType expected) = 0;

    virtual std::vector<Type *> flatten() = 0;


    virtual std::pair<Type &, Type &> find_difference(Type &against) = 0;
    std::shared_ptr<Type> self;

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
    template<class T>
    static std::shared_ptr <T> create_heaped(T obj) {

        static_assert(std::is_base_of<Type, T>(), "You may only create a heap allocation of a type derivative");
        // O(n) cache mechanism
        // so we do not have to impl a
        // custom hashing fn for each type
        // TODO: optimize
        static std::vector <T> keys;
        static std::vector <std::shared_ptr<T>> values;
        auto idx = indexof(keys, obj);
        if (idx == -1) {
            auto alloc = std::make_shared<T>(obj);
            keys.push_back(obj);
            values.push_back(alloc);
            alloc->self = alloc;
            return alloc;
        }
        return values[idx];
    }
};

class Primitive : public Type {
protected:
    Type *drill() {
        return this;
    }


public:
    PrimitiveType kind;

    std::vector<Type *> flatten() {
        return {this};
    }

    Primitive(PrimitiveType kind) {
        this->kind = kind;
    }

    std::string str() {
        return primitive_names[kind];
    }

    bool is(PrimitiveType kind) {
        return this->kind == kind;
    }

    int distance_from(Type &target) {
        auto prim = target.get<Primitive>();
        if (!prim)return -1;
        if (prim->kind == this->kind)
            return 0;

        if (primitive_group_mappings[prim->kind] == primitive_group_mappings[kind])
            return 1;
        return -1;
    }

    static std::shared_ptr <Primitive> get(PrimitiveType type) {
        return create_heaped(Primitive(type));
    }

    std::shared_ptr <Type> pointee() {
        std::shared_ptr <Type> ret(nullptr);
        return ret;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto aty = against.get<Primitive>()) {
            if (aty->kind == this->kind)return {*this, *this};
        }
        return {*this, against};
    }

    llvm::Type *internal_llvm_type() {
#define T(sw, name) \
    case PR_##sw:   \
        return builder()->get##name##Ty();
        switch (kind) {
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
            case PR_string:except(E_INTERNAL, "Strings not implemented");
            default:except(E_INTERNAL, "Failed to generate primitive type");
        }
    }

#undef T

    bool operator==(Type &against) {
        if (auto k = against.get<Primitive>()) {
            return k->kind == kind;
        }
        return false;
    }

    llvm::Constant *default_value(LLVMType expected) {
        llvm::Constant *ret = nullptr;
        switch (kind) {
            case PR_int8:
            case PR_uint8:ret = builder()->getInt8(0);
                break;
            case PR_int16:
            case PR_uint16:ret = builder()->getInt16(0);
                break;
            case PR_int32:
            case PR_uint32:ret = builder()->getInt32(0);
                break;
            case PR_int64:
            case PR_uint64:ret = builder()->getInt64(0);
                break;
            default: except(E_INTERNAL, "Cannot get default value for type: " + str());
        }

        auto op = llvm::CastInst::getCastOpcode(ret, true, expected, true);
        return llvm::ConstantExpr::getCast(op, ret, expected, false);
    }

private:
    static inline std::map <PrimitiveType, std::string> primitive_group_mappings{
            PRIMITIVES_ENUM_GROUPS};
};

class Ptr : public Type {
protected:
    Type *drill() {
        return this;
    }

public:
    Ptr(std::shared_ptr <Type> type) {
        this->of = type->drill()->self;
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }


    std::shared_ptr <Type> of;

    std::string str() {
        return of->str() + "*";
    }

    llvm::Type *internal_llvm_type() {
        return of->internal_llvm_type()->getPointerTo();
    }

    std::shared_ptr <Type> pointee() {
        return of;
    }

    static std::shared_ptr <Ptr> get(std::shared_ptr <Type> to) {
        return create_heaped(Ptr(to));
    }

    bool operator==(Type &against) {
        if (auto k = against.get<Ptr>()) {
            return k->of == of;
        }
        return false;
    }

    int distance_from(Type &target) {
        auto pt = target.get<Ptr>();
        if (!pt)
            return -1;
        return this->of->distance_from(*pt->of);
    }

    llvm::Constant *default_value(LLVMType expected) {
        return llvm::ConstantExpr::getIntToPtr(builder()->getInt64(0), expected, false);
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<Ptr>()) {
            return of->drill()->find_difference(*pty->of);
        }
        return {*this, against};
    }
};

//
// References are essentially pointers, but with the following differences
// - Guaranteed to point to valid memory
// - Members can be accessed directly (implicitly casts to pointed type)
// - Can be explicitly cast to c-style pointer for glibc compatibility
// Inspired by Rust's References
//
class ReferenceType : public Type {
public:
    std::shared_ptr <Type> of;

    ReferenceType(std::shared_ptr <Type> of) {
        this->of = of->drill()->self;
    }

    static std::shared_ptr <ReferenceType> get(std::shared_ptr <Type> of) {
        return create_heaped(ReferenceType(of));
    }

    std::string str() {
        return of->str() + "&";
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        for (auto t: of->flatten())ret.push_back(t);
        return ret;

    }

    Type *drill() {
        return this;
    }

    std::shared_ptr <Type> pointee() {
        return of;
    }

    bool operator==(Type &against) {
        if (auto k = against.get<ReferenceType>()) {
            return k->of == of;
        }
        return false;
    }

    int distance_from(Type &target) {
        auto pt = target.get<ReferenceType>();
        if (!pt)
            return -1;
        return this->of->distance_from(*pt->of->drill());
    }

    llvm::Constant *default_value(LLVMType expected) {
        except(E_BAD_TYPE, "A Reference Type must be explicitly initialized");
    }

    llvm::Type *internal_llvm_type() {
        auto of_ty = of->internal_llvm_type();
        auto pointed_ty = of_ty->getPointerTo();
        return pointed_ty;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<ReferenceType>()) {
            return of->drill()->find_difference(*pty->of);
        }
        return {*this, against};
    }
};


/*
 * Dynamically sized list type
 * uses a fat pointer to track length
 */
class DynListType : public Type {
protected:
    Type *drill() {
        // return this with a drilled `of` type (prevents bug with casting to generic slices)
        auto drilled_of = of->drill();
        if(drilled_of == of.get())return this;
        auto new_ptr = DynListType::get(drilled_of->self).get();
        return new_ptr;
    }

public:

    std::shared_ptr <Type> of;

    DynListType(std::shared_ptr <Type> type) {
        this->of = type->drill()->self;
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }

    std::shared_ptr <Type> pointee() {
        return of;
    }

    std::string str() {
        return of->str() + "[]";
    }

    static std::shared_ptr <DynListType> get(std::shared_ptr <Type> of) {
        return create_heaped(DynListType(of));
    }

    llvm::Type *internal_llvm_type() {
        if (type)return type;
        auto struct_ty = llvm::StructType::create(*llctx(), {
                builder()->getInt64Ty(), // Size of the slice
                llvm::ArrayType::get(this->of->internal_llvm_type(), 0)->getPointerTo() // Slice Elements
        }, "slice:" + this->of->str());
        type = struct_ty;
        return struct_ty;
    }

    bool operator==(Type &against) {
        if (auto k = against.get<DynListType>()) {
            return k->of == of;
        }
        return false;
    }

    int distance_from(Type &target) {
        auto lt = target.get<DynListType>();
        if (!lt)
            return -1;
        return this->of->distance_from(*lt->of);
    }

    llvm::Constant *default_value(LLVMType expected) {
        except(E_INTERNAL, "default value not implemented for list types");
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<DynListType>()) {
            return this->of->find_difference(*pty->of);
        }
        return {*this, against};
    }

private:
    llvm::StructType *type = nullptr;

};
class ParentAwareType : public Type {
public:
    Container *parent = nullptr;

    std::string get_name();
};

class StructType : public ParentAwareType {
public:
    std::map <std::string, std::shared_ptr<Type>> members;

    StructType(std::map <std::string, std::shared_ptr<Type>> members, Container *cont) {

        this->members = members;
        this->parent = cont;

    }

    Type *drill() {
        return this;
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        for (auto m: members) {
            for (auto f: m.second->flatten())ret.push_back(f);
        }
        return ret;
    }

    std::string str() {
        std::string name = "struct:{";
        bool first = true;
        for (auto member: members) {
            if (!first)name += ",";

            name += member.first + ":" + member.second->str();

            first = false;
        }
        name += "}";
        return name;
    }

    std::shared_ptr <Type> pointee() {
        return std::shared_ptr<Type>(nullptr);
    }

    int member_idx(std::string name) {
        int i = 0;
        for (auto m: members) {
            if (m.first == name)return i;
            i++;
        }
        return -1;
    }

    static std::shared_ptr <StructType> get(std::map <std::string, std::shared_ptr<Type>> members, Container *cont) {
        return create_heaped(StructType(members, cont));
    }

    int distance_from(Type &target) {
        if (!target.get<StructType>())return -1;
        if (target.get<StructType>() == this)return 0;

        // Distance is equal to that of the highest
        except(E_INTERNAL, "distance between: " + str() + " and " + target.str());
    }

    llvm::Constant *default_value(LLVMType expected) {
        except(E_INTERNAL, "default value not implemented for struct types");
    }

    bool operator==(Type &against) {
        auto st = against.get<StructType>();
        if (!st)return false;
        return st->members == members && parent == st->parent;
    }
    llvm::StructType* cached_struct_ty = nullptr;
    llvm::Type *internal_llvm_type() {
        if(cached_struct_ty)return cached_struct_ty;
        std::vector < llvm::Type * > member_types;
        for (auto member: members) {
            member_types.push_back(member.second->internal_llvm_type());
        }
        auto struct_ty = llvm::StructType::create(*llctx(), member_types, "struct:"+this->get_name());
        cached_struct_ty = struct_ty;
        return struct_ty;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        except(E_INTERNAL, "find_difference not implemented for structs");
    }
};

class EnumType : public ParentAwareType {
public:
    std::vector <std::string> entries;

    EnumType(std::vector <std::string> entries, Container *cont) {
        this->entries = entries;
        this->parent = cont;

    }

    Type *drill() {
        return this;
    }

    std::vector<Type *> flatten() {
        return {this};
    }

    std::string str() {
        std::string name = "enum:{";
        bool first = true;
        for (auto e: entries) {
            if (!first)name += ",";

            name += e;

            first = false;
        }
        name += "}";
        return name;
    }

    std::shared_ptr <Type> pointee() {
        return std::shared_ptr<Type>(nullptr);
    }

    static std::shared_ptr <EnumType> get(std::vector <std::string> entries, Container *cont) {
        return create_heaped(EnumType(entries, cont));
    }

    int distance_from(Type &target) {
        if (auto _tgt = target.get<EnumType>()) {
            if (_tgt == this)return 0;
            return -1;
        } else return -1;
        except(E_INTERNAL, "Type distance not implemented for enums");
    }

    llvm::Constant *default_value(LLVMType expected) {
        except(E_INTERNAL, "default value not implemented for enums");
    }

    bool operator==(Type &against) {
        auto en = against.get<EnumType>();
        if (!en)return false;
        return en->entries == entries && parent == en->parent;
    }

    llvm::Type *internal_llvm_type() {
        return builder()->getInt32Ty();
    }

    std::map<std::string, llvm::Constant *> get_members() {
        std::map < std::string, llvm::Constant * > ret;
        int i = 0;
        for (auto member: entries) {
            auto val = builder()->getInt32(i);
            ret[member] = val;
            i++;
        }
        return ret;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        except(E_INTERNAL, "find_difference not implemented for enums");
    }
};

class TypeRef : public Type {
public:
    std::unique_ptr <LongName> name;
    std::shared_ptr <Type> resolves_to;

    Type *drill() {
        if (resolves_to)
            return resolves_to->drill();
        return this;
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        if (resolves_to)for (auto m: resolves_to->flatten())ret.push_back(m);
        return ret;
    }

    llvm::Constant *default_value(LLVMType expected) {
        if (resolves_to)return resolves_to->default_value(expected);
        except(E_INTERNAL, "cannot get default value for unresolved type reference");
    }

    std::string str() {
        if (resolves_to)
            return resolves_to->str();
        else
            return "unresolved_t:" + name->str();
    }

    std::shared_ptr <Type> pointee() {
        return resolves_to ? resolves_to->pointee() : std::shared_ptr<Type>(nullptr);
    }

    static std::shared_ptr <TypeRef> get(std::unique_ptr <LongName> name) {
        // do not cache TypeRefs, as multiple refs may share the same name
        // but refer to an entirely different type depending on context

        auto pt = std::make_shared<TypeRef>();
        pt->name = std::move(name);
        pt->self = pt;
        return pt;
    }

    llvm::Type *internal_llvm_type() {
        if (resolves_to)
            return resolves_to->internal_llvm_type();
        except(E_UNRESOLVED_TYPE, "Cannot generate llvm value for unresolved type reference: " + name->str());
    }

    bool operator==(Type &against) {
        if (!resolves_to)
            return false;
        return *resolves_to == against;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (resolves_to)return resolves_to->find_difference(*against.drill());
        return {*this, against};
    }

    int distance_from(Type &target) {
        if (!resolves_to)
            return -1;
        return resolves_to->distance_from(target);
    }
};

class Method;

class Generic : public Type {
private:
    Type *drill() {
        return temporarily_resolves_to ? temporarily_resolves_to->drill() : this;
    }

public:
    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        if (temporarily_resolves_to)ret.push_back(temporarily_resolves_to.get());
        return ret;
    }

    llvm::Type *internal_llvm_type() {
        if (temporarily_resolves_to)return temporarily_resolves_to->internal_llvm_type();
        else except(E_UNRESOLVED_TYPE, "Cannot get an llvm type for an unresolved generic");
    }

    std::unique_ptr <Name> name;
    Method *parent;

    std::shared_ptr <Type> temporarily_resolves_to;

    Generic(std::unique_ptr <Name> name, Method *parent) {
        this->name = std::move(name);
        this->parent = parent;
    }

    static std::shared_ptr <Generic> get(std::unique_ptr <Name> name, Method *parent) {
        auto gen = std::make_shared<Generic>(std::move(name), parent);
        gen->self = gen;
        return gen;
    }

    int distance_from(Type &target) {
        auto lt = target.get<Generic>();
        if (!lt)
            return -1;
        return this->temporarily_resolves_to && (lt->temporarily_resolves_to == this->temporarily_resolves_to)
               ? this->temporarily_resolves_to->distance_from(*lt) : -1;
    }

    std::string str() {
        if (temporarily_resolves_to)return temporarily_resolves_to->str();
        else return "GENERIC:" + name->str();
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<Generic>()) {
            if (*pty == *this)return {*this, *this};
        }
        return {*this, against};
    }

    bool operator==(Type &against) {
        if (auto gen = against.get<Generic>()) {
            return gen == this;
        } else return false;
    }

    std::shared_ptr <Type> pointee() {
        if (temporarily_resolves_to)return temporarily_resolves_to->pointee();
        else except(E_UNRESOLVED_TYPE, "Cannot get pointee of unresolved generic type");
    }

    llvm::Constant *default_value(LLVMType expected) {
        if (temporarily_resolves_to)return temporarily_resolves_to->default_value(expected);
        else except(E_UNRESOLVED_TYPE, "Cannot get default value of unresolved generic type");
    }

};

#include "./constant.hh"

class ListType : public Type {
protected:
    Type *drill() {
        // return this with a drilled `of` type (prevents bug with casting to generic arrays)
        auto drilled_of = of->drill();
        if(drilled_of == of.get())return this;
        auto new_ptr = ListType::get(drilled_of->self, Integer::get(size->value)).get();
        return new_ptr;
    }

public:
    std::shared_ptr <Type> of;
    std::unique_ptr <Integer> size;

    ListType(std::shared_ptr <Type> type, std::unique_ptr <Integer> size) {
        this->of = type->drill()->self;
        this->size = std::move(size);
    }

    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }

    ListType(const ListType &from) {
        this->of = from.of;
        this->size = std::make_unique<Integer>(from.size->value);
    }

    ListType(ListType &&) = default;

    std::shared_ptr <Type> pointee() {
        return of;
    }

    std::string str() {
        return (of ? of->str() : "?") + "[" + size->str() + "]";
    }

    static std::shared_ptr <ListType> get(std::shared_ptr <Type> of, std::unique_ptr <Integer> size) {
        return create_heaped(ListType(of, std::move(size)));
    }

    llvm::Type *internal_llvm_type() {
        return llvm::ArrayType::get(of->internal_llvm_type(), size->value);
    }

    bool operator==(Type &against) {
        if (auto k = against.get<ListType>()) {
            return k->of == of && k->size->value == size->value;
        }
        return false;
    }

    int distance_from(Type &target) {
        auto lt = target.get<ListType>();
        if (!lt)
            return -1;
        if (lt->size->value != this->size->value)return -1;
        return this->of->distance_from(*lt->of);
    }

    llvm::Constant *default_value(LLVMType expected) {
        except(E_INTERNAL, "default value not implemented for list types");
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<ListType>()) {
            if (pty->size == this->size)return this->of->find_difference(*pty->of);
        }
        return {*this, against};
    }
};
