#pragma once

#include "./include.hh"
#include "llvm/IR/Type.h"
#include "./ast.hh"
#include "../../GenMacro.h"
#include "../llvm_utils.h"
#include "../token/token.h"
#include "../../lib/list.h"
#include "./reference.hh"

class Type;
typedef std::map<std::string, std::shared_ptr<Type>> GenericTable;

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
    virtual LLVMType llvm_type(GenericTable generics = {}) = 0;


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

    virtual std::vector<Type *> flatten() = 0;

    virtual std::shared_ptr<Type> copy_with_substitutions(GenericTable gt) = 0;

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
    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return self;
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

    bool is_signed_integer(){
        return is(PR_int8) || is(PR_int16) || is(PR_int32) || is(PR_int64);
    }
    bool is_unsigned_integer(){
        return is(PR_uint8) || is(PR_uint16) || is(PR_uint32) || is(PR_uint64);
    }
    bool is_integer(){
        return is_signed_integer() || is_unsigned_integer();
    }

    bool is_bool(){
        return is(PR_boolean);
    }

    bool is_float(){
        return is(PR_float16) || is(PR_float32) || is(PR_float64);
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

    LLVMType llvm_type(GenericTable gen_table = {}) {
#define T(sw, name) \
    case PR_##sw:   \
        return {builder()->get##name##Ty(), this->self};
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

private:
    static inline std::map <PrimitiveType, std::string> primitive_group_mappings{
            PRIMITIVES_ENUM_GROUPS};
};

class Ptr : public Type {
protected:
    Type *drill() {
        if(of->drill()->self == of)return this;
        return Ptr::get(of->drill()->self).get();
    }

public:
    Ptr(std::shared_ptr <Type> type) {
        this->of = type->drill()->self;
    }

    
    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return Ptr::get(this->of->copy_with_substitutions(gt));
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

    LLVMType llvm_type(GenericTable gen_table = {}) {
        return {of->llvm_type()->getPointerTo(), self};
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
        return this->of->drill()->distance_from(*pt->of->drill());
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

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return ReferenceType::get(of->copy_with_substitutions(gt));    
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
        return this->of->drill()->distance_from(*pt->of->drill());
    }

    LLVMType llvm_type(GenericTable gen_table = {}) {
        auto of_ty = of->llvm_type(gen_table);
        auto pointed_ty = of_ty->getPointerTo();
        return {pointed_ty, self};
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

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return DynListType::get(of->copy_with_substitutions(gt));
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

    LLVMType llvm_type(GenericTable gen_table = {}) {
        if (type)return {type, this->self};
        auto struct_ty = llvm::StructType::create(*llctx(), {
                builder()->getInt64Ty(), // Size of the slice
                llvm::ArrayType::get(this->of->llvm_type(gen_table), 0)->getPointerTo() // Slice Elements
        }, "slice:" + this->of->str());
        type = struct_ty;
        return {struct_ty, this->self};
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

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<DynListType>()) {
            return this->of->find_difference(*pty->of);
        }
        return {*this, against};
    }

private:
    llvm::StructType *type = nullptr;

};

class TypeMember;
class ParentAwareType : public Type {
public:
    Container *parent = nullptr;

    Type* self_ptr = nullptr;
    std::string get_name();
    TypeMember* as_member();
};

class StructType : public ParentAwareType {
public:
    std::map <std::string, std::shared_ptr<Type>> members;

    StructType(std::map <std::string, std::shared_ptr<Type>> members, Container *cont) {

        this->members = members;
        this->parent = cont;

    }

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      std::map<std::string, std::shared_ptr<Type>> new_members;

      for(auto [name, typ] : members){

        auto new_typ = typ->copy_with_substitutions(gt);
        new_members[name] = new_typ;
      }
      auto new_struct = StructType::get(new_members, parent);
      new_struct->self_ptr = this;
      return new_struct;
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

    bool operator==(Type &against) {
        auto st = against.get<StructType>();
        if (!st)return false;
        return st->members == members && parent == st->parent;
    }
    std::map<std::vector<llvm::Type*>, llvm::StructType*> type_cache;

    LLVMType llvm_type(GenericTable gen_table = {});



    std::pair<Type &, Type &> find_difference(Type &against) {
        except(E_INTERNAL, "find_difference not implemented for structs");
    }

    bool is_generic();
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

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return self;
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

    bool operator==(Type &against) {
        auto en = against.get<EnumType>();
        if (!en)return false;
        return en->entries == entries && parent == en->parent;
    }

    LLVMType llvm_type(GenericTable gen_table = {}) {
        return {builder()->getInt32Ty(), self};
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

    std::string str() {
        if (resolves_to)
            return resolves_to->str();
        else
            return "unresolved_t:" + name->str();
    }

    std::shared_ptr <Type> pointee() {
        return resolves_to ? resolves_to->pointee() : std::shared_ptr<Type>(nullptr);
    }

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return resolves_to ? resolves_to->copy_with_substitutions(gt) : self;
    }

    static std::shared_ptr <TypeRef> get(std::unique_ptr <LongName> name) {
        // do not cache TypeRefs, as multiple refs may share the same name
        // but refer to an entirely different type depending on context

        auto pt = std::make_shared<TypeRef>();
        pt->name = std::move(name);
        pt->self = pt;
        return pt;
    }

    LLVMType llvm_type(GenericTable generics = {}){
        if(resolves_to)return resolves_to->llvm_type(generics);
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

//
// Essentially a reference to a custom type
// such as structs, containing generic args
//
class ParameterizedTypeRef : public Type{
public:
    std::shared_ptr <Type> resolves_to;

    std::vector<std::shared_ptr<Type>> params;

    GenericTable get_mapped_params();
    ParameterizedTypeRef(std::shared_ptr<Type> resolves_to, std::vector<std::shared_ptr<Type>> params){
        this->resolves_to = resolves_to;
        this->params = params;
    }

    Type *drill() {
        return this;
    }

    std::shared_ptr<Type> copy_with_substitutions(GenericTable gt){
      return resolves_to ? resolves_to->copy_with_substitutions(get_mapped_params())->copy_with_substitutions(gt) : self;
    }
    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        if (resolves_to)for (auto m: resolves_to->flatten())ret.push_back(m);
        for(auto p : params)for(auto t : p->flatten())ret.push_back(t);
        return ret;
    }

    std::string str() {
        std::string name = (resolves_to ? resolves_to->str() : "unknown") + "<";

        bool first = true;
        for(auto g : this->params){
            if(!first)name+=",";
            name+=g->str();
            first = false;
        }
        name+=">";
        return name;
    }

    std::shared_ptr <Type> pointee() {
        return resolves_to ? resolves_to->pointee() : std::shared_ptr<Type>(nullptr);
    }

    static std::shared_ptr <ParameterizedTypeRef> get(std::shared_ptr<Type> resolves_to, std::vector<std::shared_ptr<Type>> params) {
        return create_heaped(ParameterizedTypeRef(resolves_to, params));
    }
    LLVMType llvm_type(GenericTable generics){
        // return the substituted version of the child
        auto mp = this->get_mapped_params();
        return this->resolves_to->llvm_type(mp);
    }

    bool operator==(Type &against) {
        auto apt = against.get<ParameterizedTypeRef>();
        if(!apt)return false;
        if(apt->resolves_to != this->resolves_to)return false;
        if(apt->params.size() != this->params.size())return false;

        for(unsigned int i = 0; i < apt->params.size(); i++){
            if(apt->params[i] != this->params[i])return false;
        }
        return true;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        except(E_INTERNAL, "find_difference not implemented for parameterized type ref");
        if (resolves_to)return resolves_to->find_difference(*against.drill());
        return {*this, against};
    }

    int distance_from(Type &target) {
        auto tptr = target.get<ParameterizedTypeRef>();
        if(!tptr)return -1;
        if(resolves_to->distance_from(*tptr->resolves_to) == -1)return -1;
        Logger::debug("Distance from: " + tptr->str() + " to " + str());
        Logger::debug("distance from refs: " + std::to_string(this->resolves_to->distance_from(*tptr->resolves_to)));
        return 0;
        except(E_INTERNAL, "distance no impl");
    }
};

class Method;

class Generic : public Type {
private:
    Type *drill() {
        return temporarily_resolves_to ? temporarily_resolves_to->drill() : this;
    }

public:


    std::shared_ptr <Type> copy_with_substitutions(GenericTable gt) {
        return temporarily_resolves_to ? temporarily_resolves_to->copy_with_substitutions(gt) : gt[name->str()] ? gt[name->str()] : self;
    }
    std::vector<Type *> flatten() {
        std::vector < Type * > ret = {this};
        if (temporarily_resolves_to)ret.push_back(temporarily_resolves_to.get());
        return ret;
    }

    LLVMType llvm_type(GenericTable gen_table = {}) {
        if (temporarily_resolves_to)return temporarily_resolves_to->llvm_type(gen_table);
        else{

            if(auto ret = gen_table[this->name->str()]){
                Logger::debug("Load generic type from gen_table");
                return ret->llvm_type(gen_table);
            }

            except(E_INTERNAL, "Cannot get an llvm type for an unresolved generic " + this->str());
        }
    }

    std::unique_ptr <Name> name;
    std::shared_ptr<Type>  constraint;
    std::shared_ptr <Type> temporarily_resolves_to;

    Generic(std::unique_ptr <Name> name) {
        this->name = std::move(name);
    }

    static std::shared_ptr <Generic> get(std::unique_ptr <Name> name) {
        auto gen = std::make_shared<Generic>(std::move(name));
        gen->self = gen;
        return gen;
    }

    int distance_from(Type &target) {
        auto lt = target.get<Generic>();
        if (!lt)
            return -1;

      // TODO: once constraints are implemented, check if this type has a constraint which conforms to the target type
      return 0;
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

    std::shared_ptr <Type> copy_with_substitutions(GenericTable gt) {
        return ListType::get(of->copy_with_substitutions(gt), Integer::get(size->value));
    }

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

    LLVMType llvm_type(GenericTable gen_table = {}) {
        return {llvm::ArrayType::get(of->llvm_type(gen_table), size->value), self};
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

    std::pair<Type &, Type &> find_difference(Type &against) {
        if (auto pty = against.get<ListType>()) {
            if (pty->size == this->size)return this->of->find_difference(*pty->of);
        }
        return {*this, against};
    }
};
