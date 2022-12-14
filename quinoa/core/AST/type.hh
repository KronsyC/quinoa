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






enum PrimitiveType {
    PRIMITIVES_ENUM_MEMBERS
};
static std::map <PrimitiveType, std::string> primitive_names{PRIMITIVES_ENUM_NAMES};
static std::map <TokenType, PrimitiveType> primitive_mappings{PRIMITIVES_ENUM_MAPPINGS};



LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat = true );


//
// TODO: refactor all of the methods which return `Type*` to
// return a shared_ptr
//
class Type : public ANode {
public:
    virtual LLVMType llvm_type(GenericTable generics = {}) = 0;


    virtual _Type pointee() = 0;

    virtual std::string str() = 0;

    template<class T>
    T *get() {
        static_assert(std::is_base_of<Type, T>(), "You can only convert a type to a subtype");
        return dynamic_cast<T *>(this->drill());
    }
    virtual bool is_generic() = 0;

    virtual bool operator==(Type &compare) = 0;

    virtual int distance_from(Type &target) = 0;

    virtual Type *drill() = 0;

    virtual std::vector<Type *> flatten() = 0;

    virtual _Type clone() = 0;

    virtual std::pair<Type &, Type &> find_difference(Type &against) = 0;
    _Type self;

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
        static std::vector<T> keys;
        static std::vector<std::shared_ptr<T>> values;
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
    bool is_generic(){return false;}
    PrimitiveType kind;

    std::vector<Type *> flatten() {
        return {this};
    }
    Primitive(PrimitiveType kind) {
        this->kind = kind;
    }
    _Type clone(){
      return self;
    }
    std::string str() {
        auto name = primitive_names[kind];
        return name;
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

    _Type pointee() {
        _Type ret(nullptr);
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
    Ptr(_Type type) {
        this->of = type->drill()->self;
    }

    _Type clone(){
      return Ptr::get(of->clone());
    }
    
    bool is_generic(){return of->is_generic();}

    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }


    _Type of;

    std::string str() {
        return of->str() + "*";
    }

    LLVMType llvm_type(GenericTable gen_table = {}) {
        return {of->llvm_type(gen_table)->getPointerTo(), self};
    }

    _Type pointee() {
        return of;
    }

    static std::shared_ptr <Ptr> get(_Type to) {
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
    _Type of;

    ReferenceType(_Type of) {
        this->of = of->drill()->self;
    }

    bool is_generic(){return of->is_generic();}


    _Type clone(){
      return ReferenceType::get(of->clone());
    }
    static std::shared_ptr <ReferenceType> get(_Type of) {
        return create_heaped(ReferenceType(of));
    }

    std::string str() {
        return of->str() + "&";
    }

    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
        for (auto t: of->flatten())ret.push_back(t);
        return ret;

    }

    Type *drill() {
        return this;
    }

    _Type pointee() {
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
        if (!pt){
            if(auto ptr = target.get<Ptr>()){
              return of->drill()->distance_from(*ptr->of->drill()) * 2;
            }
            return -1;
        }
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

    bool is_generic(){return of->is_generic();}
    _Type of;
    DynListType(_Type type) {
        this->of = type->drill()->self;
    }

    _Type clone(){
      return DynListType::get(of->clone());
    }
    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }

    _Type pointee() {
        return of;
    }

    std::string str() {
        return of->str() + "[]";
    }

    static std::shared_ptr <DynListType> get(_Type of) {
        return create_heaped(DynListType(of));
    }

    LLVMType llvm_type(GenericTable gen_table = {}) {
        auto struct_ty = llvm::StructType::get(*llctx(), {
                builder()->getInt64Ty(), // Size of the slice
                llvm::ArrayType::get(this->of->llvm_type(gen_table), 0)->getPointerTo() // Slice Elements
        });
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


};

class TypeMember;

//
// TODO: Instead of inheriting from this class
// make this class a standalone type and
// add an optional field to all types
// which refers to this class
//
class ParentAwareType : public Type {
public:
    Container *parent = nullptr;

    Type* self_ptr = nullptr;
    std::string get_name();
    TypeMember* as_member();

    Type* getself(){
      return self_ptr ? self_ptr : this;
    }
};



//
// A Tuple is a sequence of values with predefined types
// represented internally by a struct
// members are accessed using subscript `tuple[I]` notation
// created using the syntax `(u32, f32)`
//
// Internally uses an anonymous struct to represent members
// a slice (DynListType) is technically a tuple `(u64, T*)`
//
class TupleType : public Type{
public:
    TypeVec members;


    TupleType(TypeVec members){
      this->members = members;
    }

    static _Type get(TypeVec members){
      return create_heaped(TupleType(members));
    }
    _Type pointee(){
      except(E_INTERNAL, "cannot get the pointee of a tuple");
    }
    std::vector<Type*> flatten(){
      std::vector<Type*> ret = {this};

      for(auto m : members)for(auto t : m->flatten())ret.push_back(t);
      return ret;
    }

    _Type clone(){
      TypeVec cloned_members;
      for(auto m : members)cloned_members.push_back(m->clone());
    
      return TupleType::get(cloned_members);
    }
    Type* drill(){return this;}

    int distance_from(Type& t){
      auto tup = t.get<TupleType>();
      if(!tup)return -1;
      if(tup->members.size() != members.size())return -1;
      int highest_diff = 0;
      for(unsigned i = 0; i < members.size(); i++){
        auto tu = tup->members[i];
        auto diff = members[i]->distance_from(*tu);
        if(diff == -1)return -1;
        if(diff > highest_diff)highest_diff = diff;
      }
      return highest_diff;
    }
    std::pair<Type&, Type&> find_difference(Type& against){
      except(E_INTERNAL, "find_difference not implemented for tuples");
    }  
    bool is_generic(){
      for(auto m : members){
        if(m->is_generic())return true;
      }
      return false;
    }

    LLVMType llvm_type(GenericTable gt){
      std::vector<llvm::Type*> member_types;
      for(auto m : members){
        member_types.push_back(m->llvm_type());
      }
      auto llty = llvm::StructType::get(*llctx(), member_types);
      return {llty, self};
    }

    std::string str(){
      std::string ret = "tuple:(";
      bool first = true;
      for(auto m : members){
        if(!first)ret+=",";
        ret+=m->str();
        first = false;
      }
      ret+=")";
      return ret;
    }

    bool operator==(Type& compare){
      auto tup = compare.get<TupleType>();
      if(!tup)return false;
      if(tup->members.size() != members.size())return -1;
      for(unsigned i = 0; i < tup->members.size(); i++){
        if(*tup->members[i] != *members[i])return false;
      }
      return true;
    }
};

//
// Unions have the same size as their largest member + 8 bytes
// they contain a prefix u64 to discriminate between different types
//
// internally represented as a { i64, iN } where N is the size
// of the largest member (in bits)
//
class UnionType : public ParentAwareType{
public:
    // TODO: Implement this
};

class StructType : public ParentAwareType {
public:
    std::map <std::string, _Type> members;

    StructType(std::map <std::string, _Type> members, Container *cont) {

        this->members = members;
        this->parent = cont;

    }


    bool is_generic(){

      for(auto m : members){
        if(m.second->is_generic())return true;  
      }
      return false;
    }
    Type *drill() {
        return this;
    }

    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
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

    _Type pointee() {
        return _Type(nullptr);
    }

    int member_idx(std::string name) {
        int i = 0;
        for (auto m: members) {
            if (m.first == name)return i;
            i++;
        }
        return -1;
    }

    static std::shared_ptr <StructType> get(std::map <std::string, _Type> members, Container *cont) {
        return create_heaped(StructType(members, cont));
    }

    int distance_from(Type &target) {
        auto tstruc = target.get<StructType>();
        if (!tstruc)return -1;
        if (tstruc == this)return 0;

        except(E_INTERNAL, "distance between: " + str() + " and " + target.str());
    }

    bool operator==(Type &against) {
        auto st = against.get<StructType>();
        if (!st)return false;
        return st->members == members && parent == st->parent;
    }
    std::map<std::vector<llvm::Type*>, llvm::StructType*> type_cache;

    LLVMType llvm_type(GenericTable gen_table = {});


    _Type clone(){
      std::map<std::string, _Type> cloned_members;
      for(auto [m_name, m_ty] : members){
        cloned_members[m_name] = m_ty->clone();
      }

      auto strct = StructType::get(cloned_members, parent);
      strct->self_ptr = this->getself();
      return strct;
    }

    std::pair<Type &, Type &> find_difference(Type &against) {
        except(E_INTERNAL, "find_difference not implemented for structs");
    }

};


struct MethodSignature;

//
// A trait is very similar to a struct, but instead of defining properties
// traits define methods that must be present
//
class TraitType : public ParentAwareType{
public:
  Vec<MethodSignature> signatures;

  LLVMType llvm_type(GenericTable gen_table = {}){
    except(E_INTERNAL, "llvm_type for traits is not implemented");
  }
};

class EnumType : public ParentAwareType {
public:
    std::vector<std::string> entries;

    EnumType(std::vector<std::string> entries, Container *cont) {
        this->entries = entries;
        this->parent = cont;

    }

    bool is_generic(){return false;}
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

    _Type pointee() {
        return _Type(nullptr);
    }

    _Type clone(){
      return self;
    }
    static std::shared_ptr <EnumType> get(std::vector<std::string> entries, Container *cont) {
        return create_heaped(EnumType(entries, cont));
    }

    int distance_from(Type &target) {
        if (auto _tgt = target.get<EnumType>()) {
            if (_tgt == this)return 0;
        } 
        return -1;
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
    _Type resolves_to;

    Type *drill() {
        if (resolves_to)
            return resolves_to->drill();
        return this;
    }

    _Type clone(){
      if(!resolves_to)except(E_INTERNAL, "Cannot clone a non-resolved TypeRef");
      return resolves_to->clone();
    }
    bool is_generic(){
      if(resolves_to)return resolves_to->is_generic();
      except(E_INTERNAL, "Cannot call is_generic on unresolved typeref");
    }
    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
        if (resolves_to)for (auto m: resolves_to->flatten())ret.push_back(m);
        return ret;
    }

    std::string str() {
        if (resolves_to)
            return resolves_to->str();
        else
            return "unresolved_t:" + name->str();
    }

    _Type pointee() {
        return resolves_to ? resolves_to->pointee() : _Type(nullptr);
    }

    static std::shared_ptr<TypeRef> get(std::unique_ptr <LongName> name) {
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
    _Type resolves_to;

    TypeVec params;

    GenericTable get_mapped_params();
    ParameterizedTypeRef(_Type resolves_to, TypeVec params){
        this->resolves_to = resolves_to;
        this->params = params;
    }

    Type *drill() {
        return this;
    }

    bool is_generic(){
      for(auto p : params){
        if(p->is_generic())return true;
      }
      return resolves_to->is_generic();
    }


    _Type clone(){
      TypeVec cloned_params;
      for(auto p : params){
        cloned_params.push_back(p->clone());
      }
      
      return ParameterizedTypeRef::get(resolves_to->clone(), cloned_params);
    }

    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
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

    _Type pointee() {
        return resolves_to ? resolves_to->pointee() : _Type(nullptr);
    }

    static std::shared_ptr <ParameterizedTypeRef> get(_Type resolves_to, TypeVec params) {
        return create_heaped(ParameterizedTypeRef(resolves_to, params));
    }
    LLVMType llvm_type(GenericTable generics){
        // return the substituted version of the child
        this->apply_generic_substitution();
        auto ll_ty = resolves_to->clone()->llvm_type();
        this->undo_generic_substitution();
        return ll_ty;
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
        return 0;
    }

    void apply_generic_substitution();
    void undo_generic_substitution();
};

class Method;

class Generic : public Type {
  public:
    Type *drill() {
        return temporarily_resolves_to ? temporarily_resolves_to->drill() : this;
    }

    bool is_generic(){return true;}

    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
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

            except(E_INTERNAL, "Cannot get an llvm type for an unresolved generic " + this->str() + " with refs: " + std::to_string(self.use_count()));
        }
    }

    _Type clone(){
      // If the generic does not resolve to anything, that is fine
      // this can occur in the case where a return type is dependant
      // on a generic in the calling function
      return temporarily_resolves_to ? temporarily_resolves_to->clone() : self;
    }
    
    std::unique_ptr <Name> name;
    _Type  constraint;
    _Type temporarily_resolves_to;

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

    _Type pointee() {
        if (temporarily_resolves_to)return temporarily_resolves_to->pointee();
        else except(E_UNRESOLVED_TYPE, "Cannot get pointee of unresolved generic type: " + this->name->str());
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
    _Type of;
    std::unique_ptr <Integer> size;

    ListType(_Type type, std::unique_ptr <Integer> size) {
        this->of = type->drill()->self;
        this->size = std::move(size);
    }

    bool is_generic(){return of->is_generic();}
    std::vector<Type *> flatten() {
        std::vector< Type * > ret = {this};
        for (auto m: of->flatten())ret.push_back(m);
        return ret;
    }

    _Type clone(){
      return ListType::get(of->clone(), std::make_unique<Integer>(size->value));
    }

    ListType(const ListType &from) {
        this->of = from.of;
        this->size = std::make_unique<Integer>(from.size->value);
    }

    ListType(ListType &&) = default;

    _Type pointee() {
        return of;
    }

    std::string str() {
        return (of ? of->str() : "?") + "[" + size->str() + "]";
    }

    static std::shared_ptr <ListType> get(_Type of, std::unique_ptr <Integer> size) {
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
